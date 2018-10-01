#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "device_detail.hpp"
#include "event_tap_utility.hpp"
#include "grabbable_state_queues_manager.hpp"
#include "hid_grabber.hpp"
#include "hid_manager.hpp"
#include "human_interface_device.hpp"
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
#include <boost/algorithm/string.hpp>
#include <deque>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_grabber final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                 std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager,
                 std::weak_ptr<console_user_server_client> weak_console_user_server_client) : dispatcher_client(weak_dispatcher),
                                                                                              weak_grabbable_state_queues_manager_(weak_grabbable_state_queues_manager),
                                                                                              manipulator_object_id_(manipulator::make_new_manipulator_object_id()),
                                                                                              profile_(nlohmann::json()),
                                                                                              led_monitor_timer_(*this) {
    manipulator_dispatcher_ = std::make_shared<manipulator::manipulator_dispatcher>();
    manipulator_timer_ = std::make_shared<manipulator::manipulator_timer>();

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

    client_connected_connection_ = virtual_hid_device_client_->client_connected.connect([this] {
      enqueue_to_dispatcher([this] {
        logger::get_logger().info("virtual_hid_device_client_ is connected");

        update_virtual_hid_keyboard();
        update_virtual_hid_pointing();
      });
    });

    client_disconnected_connection_ = virtual_hid_device_client_->client_disconnected.connect([this] {
      enqueue_to_dispatcher([this] {
        logger::get_logger().info("virtual_hid_device_client_ is disconnected");

        stop();
      });
    });

    if (auto m = weak_grabbable_state_queues_manager_.lock()) {
      grabbable_state_changed_connection_ = m->grabbable_state_changed.connect([this](auto&& registry_entry_id, auto&& grabbable_state) {
        enqueue_to_dispatcher([this, registry_entry_id, grabbable_state] {
          retry_grab(registry_entry_id, grabbable_state);
        });
      });
    }

    post_event_to_virtual_devices_manipulator_ = std::make_shared<manipulator::details::post_event_to_virtual_devices>(weak_dispatcher_,
                                                                                                                       system_preferences_,
                                                                                                                       manipulator_dispatcher_,
                                                                                                                       manipulator_timer_,
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

    input_event_arrived_connection_ = krbn_notification_center::get_instance().input_event_arrived.connect([this] {
      enqueue_to_dispatcher([this] {
        manipulate(time_utility::mach_absolute_time());
      });
    });

    // macOS 10.12 sometimes synchronize caps lock LED to internal keyboard caps lock state.
    // The behavior causes LED state mismatch because device_grabber does not change the caps lock state of physical keyboards.
    // Thus, we monitor the LED state and update it if needed.
    led_monitor_timer_.start(
        [this] {
          update_caps_lock_led();
        },
        std::chrono::milliseconds(1000));

    // hid_manager_

    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });

    hid_manager_ = std::make_unique<hid_manager>(weak_dispatcher_,
                                                 targets);

    hid_manager_->device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }
      return true;
    });

    hid_manager_->device_detected.connect([this](auto&& weak_hid) {
      enqueue_to_dispatcher([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          hid->values_arrived.connect([this, weak_hid](auto&& shared_event_queue) {
            enqueue_to_dispatcher([this, weak_hid, shared_event_queue] {
              if (auto hid = weak_hid.lock()) {
                values_arrived(hid, shared_event_queue);
              }
            });
          });

          auto grabber = std::make_shared<hid_grabber>(weak_dispatcher_,
                                                       weak_hid);

          grabber->device_grabbing.connect([this, weak_hid] {
            if (auto hid = weak_hid.lock()) {
              return is_grabbable_callback(hid);
            }
            return grabbable_state::state::ungrabbable_permanently;
          });

          grabber->device_grabbed.connect([this, weak_hid] {
            enqueue_to_dispatcher([this, weak_hid] {
              if (auto hid = weak_hid.lock()) {
                logger::get_logger().info("{0} is grabbed", hid->get_name_for_log());

                set_grabbed(hid->get_registry_entry_id(), true);

                update_caps_lock_led();

                update_virtual_hid_pointing();

                apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
              }
            });
          });

          grabber->device_ungrabbed.connect([this, weak_hid] {
            enqueue_to_dispatcher([this, weak_hid] {
              if (auto hid = weak_hid.lock()) {
                logger::get_logger().info("{0} is ungrabbed", hid->get_name_for_log());

                set_grabbed(hid->get_registry_entry_id(), false);

                if (auto m = weak_grabbable_state_queues_manager_.lock()) {
                  m->unset_first_grabbed_event_time_stamp(hid->get_registry_entry_id());
                }

                post_device_ungrabbed_event(hid->get_device_id());

                update_virtual_hid_pointing();

                apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
              }
            });
          });

          hid_grabbers_[hid->get_registry_entry_id()] = grabber;

          output_devices_json();
          output_device_details_json();

          update_virtual_hid_pointing();

          // ----------------------------------------
          async_grab_devices();
        }
      });
    });

    hid_manager_->device_removed.connect([this](auto&& weak_hid) {
      enqueue_to_dispatcher([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          auto registry_entry_id = hid->get_registry_entry_id();

          hid_grabbers_.erase(registry_entry_id);
          device_states_.erase(registry_entry_id);

          if (auto m = weak_grabbable_state_queues_manager_.lock()) {
            m->erase_queue(registry_entry_id);
          }

          output_devices_json();
          output_device_details_json();

          // ----------------------------------------

          if (hid->is_keyboard() &&
              hid->is_karabiner_virtual_hid_device()) {
            virtual_hid_device_client_->close();
            async_ungrab_devices();

            virtual_hid_device_client_->connect();
            async_grab_devices();
          }

          update_virtual_hid_pointing();

          // ----------------------------------------
          // Refresh grab state in order to apply disable_built_in_keyboard_if_exists.

          async_grab_devices();
        }
      });
    });

    hid_manager_->async_start();
  }

  virtual ~device_grabber(void) {
    detach_from_dispatcher([this] {
      stop();

      hid_manager_ = nullptr;
      hid_grabbers_.clear();

      input_event_arrived_connection_.disconnect();

      grabbable_state_changed_connection_.disconnect();

      client_connected_connection_.disconnect();
      client_disconnected_connection_.disconnect();

      post_event_to_virtual_devices_manipulator_ = nullptr;

      simple_modifications_manipulator_manager_ = nullptr;
      complex_modifications_manipulator_manager_ = nullptr;
      fn_function_keys_manipulator_manager_ = nullptr;
      post_event_to_virtual_devices_manipulator_manager_ = nullptr;

      manipulator_timer_ = nullptr;
      manipulator_dispatcher_ = nullptr;
    });
  }

  void async_start(const std::string& user_core_configuration_file_path) {
    enqueue_to_dispatcher([this, user_core_configuration_file_path] {
      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_monitor here.
      event_tap_monitor_ = std::make_unique<event_tap_monitor>();

      event_tap_monitor_->caps_lock_state_changed.connect([this](auto&& state) {
        enqueue_to_dispatcher([this, state] {
          last_caps_lock_state_ = state;
          post_caps_lock_state_changed_event(state);
          update_caps_lock_led();
        });
      });

      event_tap_monitor_->pointing_device_event_arrived.connect([this](auto&& event_type, auto&& event) {
        enqueue_to_dispatcher([this, event_type, event] {
          auto e = event_queue::event::make_pointing_device_event_from_event_tap_event();
          event_queue::entry entry(device_id(0),
                                   event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                                   e,
                                   event_type,
                                   event);

          merged_input_event_queue_->push_back_event(entry);

          krbn_notification_center::get_instance().input_event_arrived();
        });
      });

      event_tap_monitor_->async_start();

      configuration_monitor_ = std::make_unique<configuration_monitor>(user_core_configuration_file_path);

      configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
        enqueue_to_dispatcher([this, weak_core_configuration] {
          if (auto core_configuration = weak_core_configuration.lock()) {
            core_configuration_ = core_configuration;

            logger_unique_filter_.reset();
            set_profile(core_configuration->get_selected_profile());
            async_grab_devices();
          }
        });
      });

      configuration_monitor_->async_start();

      virtual_hid_device_client_->connect();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_grab_devices(void) {
    enqueue_to_dispatcher([this] {
      for (auto& pair : hid_grabbers_) {
        if (pair.second->make_grabbable_state() == grabbable_state::state::ungrabbable_permanently) {
          pair.second->async_ungrab();
        } else {
          pair.second->async_grab();
        }
      }

      enable_devices();
    });
  }

  void async_ungrab_devices(void) {
    enqueue_to_dispatcher([this] {
      for (auto& pair : hid_grabbers_) {
        pair.second->async_ungrab();
      }

      logger::get_logger().info("Connected devices are ungrabbed");
    });
  }

  void async_unset_profile(void) {
    enqueue_to_dispatcher([this] {
      profile_ = core_configuration::profile(nlohmann::json());

      manipulator_managers_connector_.invalidate_manipulators();
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
                               event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_event(entry);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

  void async_post_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
    enqueue_to_dispatcher([this, input_source_identifiers] {
      auto event = event_queue::event::make_input_source_changed_event(input_source_identifiers);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_event(entry);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

  void async_post_keyboard_type_changed_event(void) {
    enqueue_to_dispatcher([this] {
      auto keyboard_type_string = system_preferences_utility::get_keyboard_type_string(system_preferences_.get_keyboard_type());
      auto event = event_queue::event::make_keyboard_type_changed_event(keyboard_type_string);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                               event,
                               event_type::single,
                               event);

      merged_input_event_queue_->push_back_event(entry);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

private:
  class device_state final {
  public:
    device_state(void) : grabbed_(false),
                         disabled_(false) {
    }

    bool get_grabbed(void) const {
      return grabbed_;
    }

    void set_grabbed(bool value) {
      grabbed_ = value;
    }

    bool get_disabled(void) const {
      return disabled_;
    }

    void set_disabled(bool value) {
      disabled_ = value;
    }

  private:
    bool grabbed_;
    bool disabled_;
  };

  void stop(void) {
    configuration_monitor_ = nullptr;

    async_ungrab_devices();

    event_tap_monitor_ = nullptr;

    virtual_hid_device_client_->close();
  }

  std::shared_ptr<device_state> find_device_state(registry_entry_id registry_entry_id) const {
    auto it = device_states_.find(registry_entry_id);
    if (it != std::end(device_states_)) {
      return it->second;
    }
    return nullptr;
  }

  bool grabbed(registry_entry_id registry_entry_id) const {
    if (auto s = find_device_state(registry_entry_id)) {
      return s->get_grabbed();
    }
    return false;
  }

  void set_grabbed(registry_entry_id registry_entry_id, bool value) {
    auto s = find_device_state(registry_entry_id);
    if (!s) {
      s = std::make_shared<device_state>();
    }
    s->set_grabbed(value);
  }

  bool disabled(registry_entry_id registry_entry_id) const {
    if (auto s = find_device_state(registry_entry_id)) {
      return s->get_disabled();
    }
    return false;
  }

  void set_disabled(registry_entry_id registry_entry_id, bool value) {
    auto s = find_device_state(registry_entry_id);
    if (!s) {
      s = std::make_shared<device_state>();
    }
    s->set_disabled(value);
  }

  void retry_grab(registry_entry_id registry_entry_id, boost::optional<grabbable_state> grabbable_state) {
    if (auto grabber = find_hid_grabber(registry_entry_id)) {
      // Check grabbable state

      bool grabbable = false;
      if (grabbable_state &&
          grabbable_state->get_state() == grabbable_state::state::grabbable) {
        grabbable = true;
      }

      // Grab device

      if (grabbable) {
        // Call `grab` again if current_grabbable_state is `grabbable` and not grabbed yet.
        if (!grabbed(registry_entry_id)) {
          grabber->async_ungrab();
          grabber->async_grab();
        }
      } else {
        // We should `ungrab` since current_grabbable_state is not `grabbable`.
        grabber->async_ungrab();
      }
    }
  }

  std::shared_ptr<hid_grabber> find_hid_grabber(registry_entry_id registry_entry_id) {
    auto it = hid_grabbers_.find(registry_entry_id);
    if (it != std::end(hid_grabbers_)) {
      return it->second;
    }
    return nullptr;
  }

  void manipulate(absolute_time now) {
    {
      // Avoid recursive call
      std::unique_lock<std::mutex> lock(manipulate_mutex_, std::try_to_lock);

      if (lock.owns_lock()) {
        manipulator_managers_connector_.manipulate(now);

        posted_event_queue_->clear_events();
        post_event_to_virtual_devices_manipulator_->async_post_events(virtual_hid_device_client_);
      }
    }

    if (auto min = manipulator_managers_connector_.min_input_event_time_stamp()) {
      manipulator_timer_->enqueue(
          manipulator_object_id_,
          [this, min] {
            enqueue_to_dispatcher([this, min] {
              manipulate(*min);
            });
          },
          *min);
      manipulator_timer_->async_invoke(now);
    }
  }

  void values_arrived(std::shared_ptr<human_interface_device> hid,
                      std::shared_ptr<event_queue::queue> event_queue) {
    // Update grabbable_state_queue

    if (auto m = weak_grabbable_state_queues_manager_.lock()) {
      m->update_first_grabbed_event_time_stamp(*event_queue);
    }

    // Manipulate events

    if (disabled(hid->get_registry_entry_id())) {
      // Do nothing
    } else {
      for (const auto& entry : event_queue->get_entries()) {
        event_queue::entry qe(entry.get_device_id(),
                              entry.get_event_time_stamp(),
                              entry.get_event(),
                              entry.get_event_type(),
                              entry.get_original_event());

        merged_input_event_queue_->push_back_event(qe);
      }
    }

    krbn_notification_center::get_instance().input_event_arrived();
    // manipulator_managers_connector_.log_events_sizes(logger::get_logger());
  }

  void post_device_ungrabbed_event(device_id device_id) {
    auto event = event_queue::event::make_device_ungrabbed_event();
    event_queue::entry entry(device_id,
                             event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                             event,
                             event_type::single,
                             event);

    merged_input_event_queue_->push_back_event(entry);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  void post_caps_lock_state_changed_event(bool caps_lock_state) {
    event_queue::event event(event_queue::event::type::caps_lock_state_changed, caps_lock_state);
    event_queue::entry entry(device_id(0),
                             event_queue::event_time_stamp(time_utility::mach_absolute_time()),
                             event,
                             event_type::single,
                             event);

    merged_input_event_queue_->push_back_event(entry);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  grabbable_state::state is_grabbable_callback(std::shared_ptr<human_interface_device> device) const {
    if (is_ignored_device(*device)) {
      // If we need to disable the built-in keyboard, we have to grab it.
      if (device->is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        // Do nothing
      } else {
        auto message = fmt::format("{0} is ignored.", device->get_name_for_log());
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
      auto state = m->find_current_grabbable_state(device->get_registry_entry_id());

      if (!state) {
        std::string message = fmt::format("{0} is not observed yet. Please wait for a while.",
                                          device->get_name_for_log());
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
                                    device->get_name_for_log());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::key_repeating: {
              message = fmt::format("{0} is ungrabbable temporarily while a key is repeating.",
                                    device->get_name_for_log());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while any modifier flags are pressed.",
                                    device->get_name_for_log());
              break;
            }
            case grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while mouse buttons are pressed.",
                                    device->get_name_for_log());
              break;
            }
          }
          logger_unique_filter_.warn(message);

          return grabbable_state::state::ungrabbable_temporarily;
        }

        case grabbable_state::state::ungrabbable_permanently: {
          std::string message = fmt::format("{0} is ungrabbable permanently.",
                                            device->get_name_for_log());
          logger_unique_filter_.warn(message);
          return grabbable_state::state::ungrabbable_permanently;
        }

        case grabbable_state::state::device_error: {
          std::string message = fmt::format("{0} is ungrabbable temporarily by a failure of accessing device.",
                                            device->get_name_for_log());
          logger_unique_filter_.warn(message);
          return grabbable_state::state::ungrabbable_temporarily;
        }
      }
    }

    // ----------------------------------------

    return grabbable_state::state::grabbable;
  }

  void device_disabled(human_interface_device& device) {
    // Post device_ungrabbed event in order to release modifier_flags.
    post_device_ungrabbed_event(device.get_device_id());
  }

  void event_tap_pointing_device_event_callback(CGEventType type, CGEventRef event) {
  }

  void update_caps_lock_led(void) {
    if (last_caps_lock_state_) {
      if (core_configuration_) {
        for (const auto& pair : hid_grabbers_) {
          if (auto hid = pair.second->get_weak_hid().lock()) {
            auto& di = hid->get_connected_device()->get_identifiers();
            bool manipulate_caps_lock_led = core_configuration_->get_selected_profile().get_device_manipulate_caps_lock_led(di);
            if (grabbed(hid->get_registry_entry_id()) &&
                manipulate_caps_lock_led) {
              hid->async_set_caps_lock_led_state(*last_caps_lock_state_ ? led_state::on : led_state::off);
            }
          }
        }
      }
    }
  }

  bool is_pointing_device_grabbed(void) const {
    for (const auto& pair : hid_grabbers_) {
      if (auto hid = pair.second->get_weak_hid().lock()) {
        if (hid->is_pointing_device() &&
            grabbed(hid->get_registry_entry_id())) {
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

      virtual_hid_device_client_->initialize_virtual_hid_keyboard(properties);
    }
  }

  void update_virtual_hid_pointing(void) {
    if (virtual_hid_device_client_->is_connected()) {
      if (is_pointing_device_grabbed() ||
          manipulator_managers_connector_.needs_virtual_hid_pointing()) {
        virtual_hid_device_client_->initialize_virtual_hid_pointing();
        return;
      }

      virtual_hid_device_client_->terminate_virtual_hid_pointing();
    }
  }

  bool is_ignored_device(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_ignore(device.get_connected_device()->get_identifiers());
    }

    return false;
  }

  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_disable_built_in_keyboard_if_exists(device.get_connected_device()->get_identifiers());
    }
    return false;
  }

  bool need_to_disable_built_in_keyboard(void) const {
    for (const auto& hid : hid_manager_->copy_hids()) {
      if (get_disable_built_in_keyboard_if_exists(*hid)) {
        return true;
      }
    }
    return false;
  }

  void enable_devices(void) {
    if (hid_manager_) {
      for (const auto& hid : hid_manager_->copy_hids()) {
        if (hid->is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
          set_disabled(hid->get_registry_entry_id(), true);
        } else {
          set_disabled(hid->get_registry_entry_id(), false);
        }
      }
    }
  }

  void output_devices_json(void) const {
    if (hid_manager_) {
      connected_devices connected_devices;
      for (const auto& hid : hid_manager_->copy_hids()) {
        connected_devices.push_back_device(*(hid->get_connected_device()));
      }

      auto file_path = constants::get_devices_json_file_path();
      connected_devices.async_save_to_file(file_path);
    }
  }

  void output_device_details_json(void) const {
    if (hid_manager_) {
      std::vector<device_detail> device_details;
      for (const auto& hid : hid_manager_->copy_hids()) {
        device_details.push_back(hid->make_device_detail());
      }

      std::sort(std::begin(device_details),
                std::end(device_details),
                [](auto& a, auto& b) {
                  return a.compare(b);
                });

      auto file_path = constants::get_device_details_json_file_path();
      json_utility::async_save_to_file(nlohmann::json(device_details), file_path, 0755, 0644);
    }
  }

  void set_profile(const core_configuration::profile& profile) {
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

  std::shared_ptr<manipulator::details::conditions::base> make_device_if_condition(const core_configuration::profile::device& device) const {
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
                                                             manipulator::details::to_event_definition(to_json),
                                                             manipulator_dispatcher_,
                                                             manipulator_timer_);
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
                                                                    manipulator.get_parameters(),
                                                                    manipulator_dispatcher_,
                                                                    manipulator_timer_);
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
                                                                         manipulator::details::to_event_definition(to_json),
                                                                         manipulator_dispatcher_,
                                                                         manipulator_timer_);
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
                                                                         manipulator::details::to_event_definition(to_json),
                                                                         manipulator_dispatcher_,
                                                                         manipulator_timer_);
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
                                                           manipulator::details::to_event_definition(to_json),
                                                           manipulator_dispatcher_,
                                                           manipulator_timer_);
    } catch (std::exception&) {
    }
    return nullptr;
  }

  std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager_;

  manipulator::manipulator_object_id manipulator_object_id_;
  std::shared_ptr<manipulator::manipulator_dispatcher> manipulator_dispatcher_;
  std::shared_ptr<manipulator::manipulator_timer> manipulator_timer_;

  std::shared_ptr<virtual_hid_device_client> virtual_hid_device_client_;
  boost::signals2::connection client_connected_connection_;
  boost::signals2::connection client_disconnected_connection_;

  boost::signals2::connection grabbable_state_changed_connection_;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<const core_configuration> core_configuration_;

  std::unique_ptr<event_tap_monitor> event_tap_monitor_;
  boost::optional<bool> last_caps_lock_state_;
  std::unique_ptr<hid_manager> hid_manager_;
  std::unordered_map<registry_entry_id, std::shared_ptr<hid_grabber>> hid_grabbers_;
  std::unordered_map<registry_entry_id, std::shared_ptr<device_state>> device_states_;

  core_configuration::profile profile_;
  system_preferences system_preferences_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;
  boost::signals2::connection input_event_arrived_connection_;

  std::mutex manipulate_mutex_;

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

  pqrs::dispatcher::extra::timer led_monitor_timer_;

  mutable logger::unique_filter logger_unique_filter_;
};
} // namespace krbn
