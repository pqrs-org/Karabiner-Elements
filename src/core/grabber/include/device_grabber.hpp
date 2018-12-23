#pragma once

#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "device_grabber_details/entry.hpp"
#include "event_tap_utility.hpp"
#include "grabbable_state_queues_manager.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "iokit_utility.hpp"
#include "json_utility.hpp"
#include "krbn_notification_center.hpp"
#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "monitor/configuration_monitor.hpp"
#include "monitor/event_tap_monitor.hpp"
#include "spdlog_utility.hpp"
#include "system_preferences_utility.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include <deque>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nod/nod.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_grabber final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager,
                 std::weak_ptr<console_user_server_client> weak_console_user_server_client) : dispatcher_client(),
                                                                                              weak_grabbable_state_queues_manager_(weak_grabbable_state_queues_manager),
                                                                                              profile_(nlohmann::json()) {
    simple_modifications_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
    complex_modifications_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
    fn_function_keys_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
    post_event_to_virtual_devices_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();

    merged_input_event_queue_ = std::make_shared<event_queue::queue>();
    simple_modifications_applied_event_queue_ = std::make_shared<event_queue::queue>();
    complex_modifications_applied_event_queue_ = std::make_shared<event_queue::queue>();
    fn_function_keys_applied_event_queue_ = std::make_shared<event_queue::queue>();
    posted_event_queue_ = std::make_shared<event_queue::queue>();

    virtual_hid_device_client_ = std::make_shared<virtual_hid_device_client>();

    virtual_hid_device_client_->client_connected.connect([this] {
      logger::get_logger().info("virtual_hid_device_client_ is connected");

      update_virtual_hid_keyboard();
      update_virtual_hid_pointing();
    });

    virtual_hid_device_client_->client_disconnected.connect([this] {
      logger::get_logger().info("virtual_hid_device_client_ is disconnected");

      stop();
    });

    if (auto m = weak_grabbable_state_queues_manager_.lock()) {
      external_signal_connections_.emplace_back(
          m->grabbable_state_changed.connect([this](auto&& device_id, auto&& grabbable_state) {
            auto it = entries_.find(device_id);
            if (it != std::end(entries_)) {
              grab_device(it->second);
            }
          }));
    }

    post_event_to_virtual_devices_manipulator_ = std::make_shared<manipulator::details::post_event_to_virtual_devices>(system_preferences_,
                                                                                                                       weak_console_user_server_client);
    post_event_to_virtual_devices_manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::details::base>(post_event_to_virtual_devices_manipulator_));

    complex_modifications_applied_event_queue_->enable_manipulator_environment_json_output(constants::get_manipulator_environment_json_file_path());

    // Connect manipulator_managers

    manipulator_managers_connector_.emplace_back_connection(simple_modifications_manipulator_manager_,
                                                            merged_input_event_queue_,
                                                            simple_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(complex_modifications_manipulator_manager_,
                                                            complex_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(fn_function_keys_manipulator_manager_,
                                                            fn_function_keys_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(post_event_to_virtual_devices_manipulator_manager_,
                                                            posted_event_queue_);

    external_signal_connections_.emplace_back(
        krbn_notification_center::get_instance().input_event_arrived.connect([this] {
          manipulate(time_utility::mach_absolute_time_point());
        }));

    // hid_manager_

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_pointer),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = make_device_id(registry_entry_id);

        if (iokit_utility::is_karabiner_virtual_hid_device(*device_ptr)) {
          return;
        }

        auto entry = std::make_shared<device_grabber_details::entry>(device_id,
                                                                     *device_ptr,
                                                                     core_configuration_);
        entries_[device_id] = entry;

        entry->get_hid_queue_value_monitor()->values_arrived.connect([this, device_id](auto&& values_ptr) {
          auto it = entries_.find(device_id);
          if (it != std::end(entries_)) {
            auto event_queue = event_queue::utility::make_queue(device_id,
                                                                iokit_utility::make_hid_values(values_ptr));
            event_queue = event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue,
                                                                                                           device_id,
                                                                                                           it->second->get_pressed_keys_manager());
            values_arrived(device_id, event_queue);
          }
        });

        entry->get_hid_queue_value_monitor()->started.connect([this, device_id] {
          auto it = entries_.find(device_id);
          if (it != std::end(entries_)) {
            logger::get_logger().info("{0} is grabbed.",
                                      it->second->get_device_name());

            post_device_grabbed_event(it->second->get_device_properties());

            it->second->set_grabbed(true);

            update_caps_lock_led();

            update_virtual_hid_pointing();

            apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
          }
        });

        entry->get_hid_queue_value_monitor()->stopped.connect([this, device_id] {
          auto it = entries_.find(device_id);
          if (it != std::end(entries_)) {
            logger::get_logger().info("{0} is ungrabbed.",
                                      it->second->get_device_name());

            it->second->set_grabbed(false);

            if (auto m = weak_grabbable_state_queues_manager_.lock()) {
              m->unset_first_grabbed_event_time_stamp(device_id);
            }

            post_device_ungrabbed_event(device_id);

            update_virtual_hid_pointing();

            apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
          }
        });

        output_devices_json();
        output_device_details_json();

        update_virtual_hid_pointing();

        // ----------------------------------------
        update_devices_disabled();
        async_grab_devices();
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = make_device_id(registry_entry_id);

      auto it = entries_.find(device_id);
      if (it != std::end(entries_)) {
        logger::get_logger().info("{0} is terminated.",
                                  it->second->get_device_name());

        if (auto device_properties = it->second->get_device_properties()) {
          if (device_properties->get_is_keyboard().value_or(false) &&
              device_properties->get_is_karabiner_virtual_hid_device().value_or(false)) {
            virtual_hid_device_client_->async_close();
            async_ungrab_devices();

            virtual_hid_device_client_->async_connect();
          }
        }

        entries_.erase(it);
      }

      if (auto m = weak_grabbable_state_queues_manager_.lock()) {
        m->erase_queue(device_id);
      }

      output_devices_json();
      output_device_details_json();

      // ----------------------------------------

      post_device_ungrabbed_event(device_id);

      update_virtual_hid_pointing();

      // ----------------------------------------
      update_devices_disabled();
      async_grab_devices();
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& iokit_return) {
      logger::get_logger().error("{0}: {1}", message, iokit_return.to_string());
    });

    hid_manager_->async_start();
  }

  virtual ~device_grabber(void) {
    detach_from_dispatcher([this] {
      stop();

      hid_manager_ = nullptr;

      external_signal_connections_.clear();

      post_event_to_virtual_devices_manipulator_ = nullptr;

      simple_modifications_manipulator_manager_ = nullptr;
      complex_modifications_manipulator_manager_ = nullptr;
      fn_function_keys_manipulator_manager_ = nullptr;
      post_event_to_virtual_devices_manipulator_manager_ = nullptr;
    });
  }

  void async_start(const std::string& user_core_configuration_file_path) {
    enqueue_to_dispatcher([this, user_core_configuration_file_path] {
      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_monitor here.
      event_tap_monitor_ = std::make_unique<event_tap_monitor>();

      event_tap_monitor_->pointing_device_event_arrived.connect([this](auto&& event_type, auto&& event) {
        auto e = event_queue::event::make_pointing_device_event_from_event_tap_event();
        event_queue::entry entry(device_id(0),
                                 event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                                 e,
                                 event_type,
                                 event);

        merged_input_event_queue_->push_back_entry(entry);

        krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
      });

      event_tap_monitor_->async_start();

      configuration_monitor_ = std::make_unique<configuration_monitor>(user_core_configuration_file_path);

      configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
        if (auto core_configuration = weak_core_configuration.lock()) {
          core_configuration_ = core_configuration;

          for (auto&& e : entries_) {
            e.second->set_core_configuration(core_configuration);
          }

          logger_unique_filter_.reset();
          set_profile(core_configuration->get_selected_profile());

          update_devices_disabled();
          async_grab_devices();
        }
      });

      configuration_monitor_->async_start();

      virtual_hid_device_client_->async_connect();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_grab_devices(void) {
    enqueue_to_dispatcher([this] {
      for (auto&& e : entries_) {
        grab_device(e.second);
      }
    });
  }

  void async_ungrab_devices(void) {
    enqueue_to_dispatcher([this] {
      for (auto&& e : entries_) {
        e.second->get_hid_queue_value_monitor()->async_stop();
      }

      logger::get_logger().info("Connected devices are ungrabbed");
    });
  }

  void async_unset_profile(void) {
    enqueue_to_dispatcher([this] {
      profile_ = core_configuration::details::profile(nlohmann::json());

      manipulator_managers_connector_.invalidate_manipulators();
    });
  }

  void async_set_caps_lock_state(bool state) {
    enqueue_to_dispatcher([this, state] {
      last_caps_lock_state_ = state;
      post_caps_lock_state_changed_event(state);
      update_caps_lock_led();
    });
  }

  void async_set_system_preferences(const system_preferences& value) {
    enqueue_to_dispatcher([this, value] {
      system_preferences_ = value;

      update_fn_function_keys_manipulators();
      async_post_keyboard_type_changed_event();
    });
  }

  void async_post_frontmost_application_changed_event(const std::string& bundle_identifier,
                                                      const std::string& file_path) {
    enqueue_to_dispatcher([this, bundle_identifier, file_path] {
      auto event = event_queue::event::make_frontmost_application_changed_event(bundle_identifier,
                                                                                file_path);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
    enqueue_to_dispatcher([this, input_source_identifiers] {
      auto event = event_queue::event::make_input_source_changed_event(input_source_identifiers);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_keyboard_type_changed_event(void) {
    enqueue_to_dispatcher([this] {
      auto keyboard_type_string = system_preferences_utility::get_keyboard_type_string(system_preferences_.get_keyboard_type());
      auto event = event_queue::event::make_keyboard_type_changed_event(keyboard_type_string);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

private:
  void stop(void) {
    configuration_monitor_ = nullptr;

    async_ungrab_devices();

    event_tap_monitor_ = nullptr;

    virtual_hid_device_client_->async_close();
  }

  // This method is executed in the shared dispatcher thread.
  void manipulate(absolute_time_point now) {
    manipulator_managers_connector_.manipulate(now);

    posted_event_queue_->clear_events();
    post_event_to_virtual_devices_manipulator_->async_post_events(virtual_hid_device_client_);

    if (auto min = manipulator_managers_connector_.min_input_event_time_stamp()) {
      auto when = when_now();
      if (now < *min) {
        when += time_utility::to_milliseconds(*min - now);
      }

      enqueue_to_dispatcher(
          [this, min] {
            manipulate(*min);
          },
          when);
    }
  }

  void values_arrived(device_id device_id,
                      std::shared_ptr<event_queue::queue> event_queue) {
    auto it = entries_.find(device_id);
    if (it != std::end(entries_)) {
      // Update grabbable_state_queue

      if (auto m = weak_grabbable_state_queues_manager_.lock()) {
        m->update_first_grabbed_event_time_stamp(*event_queue);
      }

      // Manipulate events

      if (it->second->get_disabled()) {
        // Do nothing
      } else {
        for (const auto& entry : event_queue->get_entries()) {
          event_queue::entry qe(entry.get_device_id(),
                                entry.get_event_time_stamp(),
                                entry.get_event(),
                                entry.get_event_type(),
                                entry.get_original_event());

          merged_input_event_queue_->push_back_entry(qe);
        }
      }

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
      // manipulator_managers_connector_.log_events_sizes(logger::get_logger());
    }
  }

  void post_device_grabbed_event(std::shared_ptr<device_properties> device_properties) {
    if (device_properties) {
      if (auto device_id = device_properties->get_device_id()) {
        auto event = event_queue::event::make_device_grabbed_event(*device_properties);
        event_queue::entry entry(*device_id,
                                 event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                                 event,
                                 event_type::single,
                                 event);

        merged_input_event_queue_->push_back_entry(entry);

        krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
      }
    }
  }

  void post_device_ungrabbed_event(device_id device_id) {
    auto event = event_queue::event::make_device_ungrabbed_event();
    event_queue::entry entry(device_id,
                             event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                             event,
                             event_type::single,
                             event);

    merged_input_event_queue_->push_back_entry(entry);

    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
  }

  void post_caps_lock_state_changed_event(bool caps_lock_state) {
    event_queue::event event(event_queue::event::type::caps_lock_state_changed, caps_lock_state);
    event_queue::entry entry(device_id(0),
                             event_queue::event_time_stamp(time_utility::mach_absolute_time_point()),
                             event,
                             event_type::single,
                             event);

    merged_input_event_queue_->push_back_entry(entry);

    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
  }

  void grab_device(std::shared_ptr<device_grabber_details::entry> entry) const {
    if (make_grabbable_state(entry) == grabbable_state::state::grabbable) {
      entry->async_start_queue_value_monitor();
    } else {
      entry->async_stop_queue_value_monitor();
    }
  }

  grabbable_state::state make_grabbable_state(std::shared_ptr<device_grabber_details::entry> entry) const {
    if (!entry) {
      return grabbable_state::state::ungrabbable_permanently;
    }

    if (entry->is_ignored_device()) {
      auto device_properties = entry->get_device_properties();

      // If we need to disable the built-in keyboard, we have to grab it.
      if (device_properties &&
          device_properties->get_is_built_in_keyboard().value_or(false) &&
          need_to_disable_built_in_keyboard()) {
        // Do nothing
      } else {
        auto message = fmt::format("{0} is ignored.",
                                   entry->get_device_name());
        logger_unique_filter_.info(message);
        return grabbable_state::state::ungrabbable_permanently;
      }
    }

    // ----------------------------------------
    // Ungrabbable while virtual_hid_device_client_ is not ready.

    if (!virtual_hid_device_client_->is_connected()) {
      std::string message = "virtual_hid_device_client is not connected yet. Please wait for a while.";
      logger_unique_filter_.warn(message);
      return grabbable_state::state::ungrabbable_temporarily;
    }

    if (!virtual_hid_device_client_->is_virtual_hid_keyboard_ready()) {
      std::string message = "virtual_hid_keyboard is not ready. Please wait for a while.";
      logger_unique_filter_.warn(message);
      return grabbable_state::state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Check observer state

    if (auto m = weak_grabbable_state_queues_manager_.lock()) {
      auto state = m->find_current_grabbable_state(entry->get_device_id());

      if (!state) {
        std::string message = fmt::format("{0} is not observed yet. Please wait for a while.",
                                          entry->get_device_name());
        logger_unique_filter_.warn(message);
        return grabbable_state::state::ungrabbable_temporarily;
      }

      switch (state->get_state()) {
        case grabbable_state::state::none:
          return grabbable_state::state::ungrabbable_temporarily;

        case grabbable_state::state::grabbable:
          break;

        case grabbable_state::state::ungrabbable_temporarily: {
          std::string message;
          switch (state->get_ungrabbable_temporarily_reason()) {
            case grabbable_state::ungrabbable_temporarily_reason::none: {
              message = fmt::format("{0} is ungrabbable temporarily",
                                    entry->get_device_name());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::key_repeating: {
              message = fmt::format("{0} is ungrabbable temporarily while a key is repeating.",
                                    entry->get_device_name());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while any modifier flags are pressed.",
                                    entry->get_device_name());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while mouse buttons are pressed.",
                                    entry->get_device_name());
              break;
            }
          }
          logger_unique_filter_.warn(message);

          return grabbable_state::state::ungrabbable_temporarily;
        }

        case grabbable_state::state::ungrabbable_permanently: {
          std::string message = fmt::format("{0} is ungrabbable permanently.",
                                            entry->get_device_name());
          logger_unique_filter_.warn(message);
          return grabbable_state::state::ungrabbable_permanently;
        }

        case grabbable_state::state::device_error: {
          std::string message = fmt::format("{0} is ungrabbable temporarily by a failure of accessing device.",
                                            entry->get_device_name());
          logger_unique_filter_.warn(message);
          return grabbable_state::state::ungrabbable_temporarily;
        }
      }
    }

    // ----------------------------------------

    return grabbable_state::state::grabbable;
  }

  void event_tap_pointing_device_event_callback(CGEventType type, CGEventRef event) {
  }

  void update_caps_lock_led(void) {
    std::optional<led_state> state;
    if (last_caps_lock_state_) {
      state = *last_caps_lock_state_ ? led_state::on : led_state::off;
    }

    for (auto&& e : entries_) {
      e.second->get_caps_lock_led_state_manager()->set_state(state);
    }
  }

  bool is_pointing_device_grabbed(void) const {
    for (const auto& e : entries_) {
      if (auto device_properties = e.second->get_device_properties()) {
        if (device_properties->get_is_pointing_device().value_or(false) &&
            e.second->get_grabbed()) {
          return true;
        }
      }
    }
    return false;
  }

  void update_virtual_hid_keyboard(void) {
    if (virtual_hid_device_client_->is_connected()) {
      pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
      properties.country_code = profile_.get_virtual_hid_keyboard().get_country_code();

      virtual_hid_device_client_->async_initialize_virtual_hid_keyboard(properties);
    }
  }

  void update_virtual_hid_pointing(void) {
    if (virtual_hid_device_client_->is_connected()) {
      if (is_pointing_device_grabbed() ||
          manipulator_managers_connector_.needs_virtual_hid_pointing()) {
        virtual_hid_device_client_->async_initialize_virtual_hid_pointing();
        return;
      }

      virtual_hid_device_client_->async_terminate_virtual_hid_pointing();
    }
  }

  bool need_to_disable_built_in_keyboard(void) const {
    for (const auto& e : entries_) {
      if (e.second->is_disable_built_in_keyboard_if_exists()) {
        return true;
      }
    }
    return false;
  }

  void update_devices_disabled(void) {
    for (const auto& e : entries_) {
      if (auto device_properties = e.second->get_device_properties()) {
        if (device_properties->get_is_built_in_keyboard().value_or(false) &&
            need_to_disable_built_in_keyboard()) {
          e.second->set_disabled(true);
        } else {
          e.second->set_disabled(false);
        }
      }
    }
  }

  void output_devices_json(void) const {
    connected_devices::connected_devices connected_devices;
    for (const auto& e : entries_) {
      if (auto device_properties = e.second->get_device_properties()) {
        connected_devices::details::device d(*device_properties);
        connected_devices.push_back_device(d);
      }
    }

    auto file_path = constants::get_devices_json_file_path();
    connected_devices.async_save_to_file(file_path);
  }

  void output_device_details_json(void) const {
    std::vector<device_properties> device_details;
    for (const auto& e : entries_) {
      if (auto device_properties = e.second->get_device_properties()) {
        device_details.push_back(*device_properties);
      }
    }

    std::sort(std::begin(device_details),
              std::end(device_details),
              [](auto& a, auto& b) {
                return a.compare(b);
              });

    auto file_path = constants::get_device_details_json_file_path();
    json_utility::async_save_to_file(nlohmann::json(device_details), file_path, 0755, 0644);
  }

  void set_profile(const core_configuration::details::profile& profile) {
    profile_ = profile;

    update_simple_modifications_manipulators();
    update_complex_modifications_manipulators();
    update_fn_function_keys_manipulators();

    update_virtual_hid_keyboard();
    update_virtual_hid_pointing();
  }

  void update_simple_modifications_manipulators(void) {
    simple_modifications_manipulator_manager_->invalidate_manipulators();

    for (const auto& device : profile_.get_devices()) {
      for (const auto& pair : device.get_simple_modifications().get_pairs()) {
        if (auto m = make_simple_modifications_manipulator(pair)) {
          auto c = make_device_if_condition(device);
          m->push_back_condition(c);
          simple_modifications_manipulator_manager_->push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_simple_modifications().get_pairs()) {
      if (auto m = make_simple_modifications_manipulator(pair)) {
        simple_modifications_manipulator_manager_->push_back_manipulator(m);
      }
    }
  }

  std::shared_ptr<manipulator::details::conditions::base> make_device_if_condition(const core_configuration::details::device& device) const {
    nlohmann::json json;
    json["type"] = "device_if";
    json["identifiers"] = nlohmann::json::array();
    json["identifiers"].push_back(nlohmann::json::object());
    json["identifiers"].back()["vendor_id"] = type_safe::get(device.get_identifiers().get_vendor_id());
    json["identifiers"].back()["product_id"] = type_safe::get(device.get_identifiers().get_product_id());
    json["identifiers"].back()["is_keyboard"] = device.get_identifiers().get_is_keyboard();
    json["identifiers"].back()["is_pointing_device"] = device.get_identifiers().get_is_pointing_device();
    return std::make_shared<manipulator::details::conditions::device>(json);
  }

  std::shared_ptr<manipulator::details::base> make_simple_modifications_manipulator(const std::pair<std::string, std::string>& pair) const {
    if (!pair.first.empty() && !pair.second.empty()) {
      try {
        auto from_json = nlohmann::json::parse(pair.first);
        from_json["modifiers"]["optional"] = nlohmann::json::array();
        from_json["modifiers"]["optional"].push_back("any");

        auto to_json = nlohmann::json::parse(pair.second);

        return std::make_shared<manipulator::details::basic>(manipulator::details::basic::from_event_definition(from_json),
                                                             manipulator::details::to_event_definition(to_json));
      } catch (std::exception&) {
      }
    }
    return nullptr;
  }

  void update_complex_modifications_manipulators(void) {
    complex_modifications_manipulator_manager_->invalidate_manipulators();

    for (const auto& rule : profile_.get_complex_modifications().get_rules()) {
      for (const auto& manipulator : rule.get_manipulators()) {
        auto m = manipulator::manipulator_factory::make_manipulator(manipulator.get_json(),
                                                                    manipulator.get_parameters());
        for (const auto& c : manipulator.get_conditions()) {
          m->push_back_condition(manipulator::manipulator_factory::make_condition(c.get_json()));
        }
        complex_modifications_manipulator_manager_->push_back_manipulator(m);
      }
    }
  }

  void update_fn_function_keys_manipulators(void) {
    fn_function_keys_manipulator_manager_->invalidate_manipulators();

    auto from_mandatory_modifiers = nlohmann::json::array();

    auto from_optional_modifiers = nlohmann::json::array();
    from_optional_modifiers.push_back("any");

    auto to_modifiers = nlohmann::json::array();

    if (system_preferences_.get_keyboard_fn_state()) {
      // f1 -> f1
      // fn+f1 -> display_brightness_decrement

      from_mandatory_modifiers.push_back("fn");
      to_modifiers.push_back("fn");

    } else {
      // f1 -> display_brightness_decrement
      // fn+f1 -> f1

      // fn+f1 ... fn+f12 -> f1 .. f12

      for (int i = 1; i <= 12; ++i) {
        auto from_json = nlohmann::json::object({
            {"key_code", fmt::format("f{0}", i)},
            {"modifiers", nlohmann::json::object({
                              {"mandatory", nlohmann::json::array({"fn"})},
                              {"optional", nlohmann::json::array({"any"})},
                          })},
        });

        auto to_json = nlohmann::json::object({
            {"key_code", fmt::format("f{0}", i)},
            {"modifiers", nlohmann::json::array({"fn"})},
        });

        auto manipulator = std::make_shared<manipulator::details::basic>(manipulator::details::basic::from_event_definition(from_json),
                                                                         manipulator::details::to_event_definition(to_json));
        fn_function_keys_manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
      }
    }

    // from_modifiers+f1 -> display_brightness_decrement ...

    for (const auto& device : profile_.get_devices()) {
      for (const auto& pair : device.get_fn_function_keys().get_pairs()) {
        if (auto m = make_fn_function_keys_manipulator(pair,
                                                       from_mandatory_modifiers,
                                                       from_optional_modifiers,
                                                       to_modifiers)) {
          auto c = make_device_if_condition(device);
          m->push_back_condition(c);
          fn_function_keys_manipulator_manager_->push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_fn_function_keys().get_pairs()) {
      if (auto m = make_fn_function_keys_manipulator(pair,
                                                     from_mandatory_modifiers,
                                                     from_optional_modifiers,
                                                     to_modifiers)) {
        fn_function_keys_manipulator_manager_->push_back_manipulator(m);
      }
    }

    // fn+return_or_enter -> keypad_enter ...
    {
      nlohmann::json data = nlohmann::json::array();

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "return_or_enter"}})},
          {"to", nlohmann::json::object({{"key_code", "keypad_enter"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "delete_or_backspace"}})},
          {"to", nlohmann::json::object({{"key_code", "delete_forward"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "right_arrow"}})},
          {"to", nlohmann::json::object({{"key_code", "end"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "left_arrow"}})},
          {"to", nlohmann::json::object({{"key_code", "home"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "down_arrow"}})},
          {"to", nlohmann::json::object({{"key_code", "page_down"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", "up_arrow"}})},
          {"to", nlohmann::json::object({{"key_code", "page_up"}})},
      }));

      for (const auto& d : data) {
        auto from_json = d["from"];
        from_json["modifiers"]["mandatory"] = nlohmann::json::array({"fn"});
        from_json["modifiers"]["optional"] = nlohmann::json::array({"any"});

        auto to_json = d["to"];
        to_json["modifiers"] = nlohmann::json::array({"fn"});

        auto manipulator = std::make_shared<manipulator::details::basic>(manipulator::details::basic::from_event_definition(from_json),
                                                                         manipulator::details::to_event_definition(to_json));
        fn_function_keys_manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
      }
    }
  }

  std::shared_ptr<manipulator::details::base> make_fn_function_keys_manipulator(const std::pair<std::string, std::string>& pair,
                                                                                const nlohmann::json& from_mandatory_modifiers,
                                                                                const nlohmann::json& from_optional_modifiers,
                                                                                const nlohmann::json& to_modifiers) {
    try {
      auto from_json = nlohmann::json::parse(pair.first);
      if (from_json.empty()) {
        return nullptr;
      }
      from_json["modifiers"]["mandatory"] = from_mandatory_modifiers;
      from_json["modifiers"]["optional"] = from_optional_modifiers;

      auto to_json = nlohmann::json::parse(pair.second);
      if (to_json.empty()) {
        return nullptr;
      }
      to_json["modifiers"] = to_modifiers;

      return std::make_shared<manipulator::details::basic>(manipulator::details::basic::from_event_definition(from_json),
                                                           manipulator::details::to_event_definition(to_json));
    } catch (std::exception&) {
    }
    return nullptr;
  }

  std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager_;

  std::shared_ptr<virtual_hid_device_client> virtual_hid_device_client_;

  std::vector<nod::scoped_connection> external_signal_connections_;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<const core_configuration::core_configuration> core_configuration_;

  std::unique_ptr<event_tap_monitor> event_tap_monitor_;
  std::optional<bool> last_caps_lock_state_;
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<device_id, std::shared_ptr<device_grabber_details::entry>> entries_;

  core_configuration::details::profile profile_;
  system_preferences system_preferences_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;

  std::shared_ptr<event_queue::queue> merged_input_event_queue_;

  std::shared_ptr<manipulator::manipulator_manager> simple_modifications_manipulator_manager_;
  std::shared_ptr<event_queue::queue> simple_modifications_applied_event_queue_;

  std::shared_ptr<manipulator::manipulator_manager> complex_modifications_manipulator_manager_;
  std::shared_ptr<event_queue::queue> complex_modifications_applied_event_queue_;

  std::shared_ptr<manipulator::manipulator_manager> fn_function_keys_manipulator_manager_;
  std::shared_ptr<event_queue::queue> fn_function_keys_applied_event_queue_;

  std::shared_ptr<manipulator::details::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator_;
  std::shared_ptr<manipulator::manipulator_manager> post_event_to_virtual_devices_manipulator_manager_;
  std::shared_ptr<event_queue::queue> posted_event_queue_;

  mutable logger::unique_filter logger_unique_filter_;
};
} // namespace krbn
