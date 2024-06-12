#pragma once

#include "chrono_utility.hpp"
#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "device_grabber_details/entry.hpp"
#include "device_grabber_details/fn_function_keys_manipulator_manager.hpp"
#include "device_grabber_details/simple_modifications_manipulator_manager.hpp"
#include "event_tap_utility.hpp"
#include "filesystem_utility.hpp"
#include "grabber/grabber_state_json_writer.hpp"
#include "gsl/gsl"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "iokit_utility.hpp"
#include "json_writer.hpp"
#include "krbn_notification_center.hpp"
#include "logger.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "memory_utility.hpp"
#include "monitor/configuration_monitor.hpp"
#include "monitor/event_tap_monitor.hpp"
#include "notification_message_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "types.hpp"
#include <deque>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nod/nod.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_driver.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_power_management.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/spdlog.hpp>
#include <string_view>
#include <thread>
#include <time.h>

namespace krbn {
namespace grabber {
class device_grabber final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(std::weak_ptr<console_user_server_client> weak_console_user_server_client,
                 std::weak_ptr<grabber_state_json_writer> weak_grabber_state_json_writer)
      : dispatcher_client(),
        core_configuration_(std::make_shared<core_configuration::core_configuration>()),
        system_sleeping_(false),
        logger_unique_filter_(logger::get_logger()) {
    notification_message_manager_ = std::make_shared<notification_message_manager>(
        constants::get_notification_message_file_path());

    simple_modifications_manipulator_manager_ = std::make_shared<device_grabber_details::simple_modifications_manipulator_manager>();
    complex_modifications_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
    fn_function_keys_manipulator_manager_ = std::make_shared<device_grabber_details::fn_function_keys_manipulator_manager>();
    post_event_to_virtual_devices_manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();

    merged_input_event_queue_ = std::make_shared<event_queue::queue>(
        "merged_input_event_queue");
    simple_modifications_applied_event_queue_ = std::make_shared<event_queue::queue>(
        "simple_modifications_applied_event_queue");
    complex_modifications_applied_event_queue_ = std::make_shared<event_queue::queue>(
        "complex_modifications_applied_event_queue");
    fn_function_keys_applied_event_queue_ = std::make_shared<event_queue::queue>(
        "fn_function_keys_applied_event_queue");
    posted_event_queue_ = std::make_shared<event_queue::queue>(
        "posted_event_queue");

    //
    // virtual_hid_device_service_client_
    //

    virtual_hid_device_service_client_ = std::make_shared<pqrs::karabiner::driverkit::virtual_hid_device_service::client>();

    virtual_hid_device_service_client_->connected.connect([this] {
      logger::get_logger()->info("virtual_hid_device_service_client_ connected");

      update_virtual_hid_keyboard();
      update_virtual_hid_pointing();

      update_devices_disabled();
      async_grab_devices();
    });

    virtual_hid_device_service_client_->connect_failed.connect([](auto&& error_code) {
      logger::get_logger()->info("virtual_hid_device_service_client_ connect_failed: {0}", error_code.message());
    });

    virtual_hid_device_service_client_->closed.connect([] {
      logger::get_logger()->info("virtual_hid_device_service_client_ closed");

      // Wait automatic reconnection.
      //
      // Note:
      // The following callback will be signaled by virtual_hid_device_service::client.
      // - `virtual_hid_keyboard_ready_response(false)`
      // - `virtual_hid_pointing_ready_response(false)`
    });

    virtual_hid_device_service_client_->error_occurred.connect([](auto&& error_code) {
      logger::get_logger()->info("virtual_hid_device_service_client_ error_occurred: {0}", error_code.message());
    });

    virtual_hid_device_service_client_->driver_activated.connect([weak_grabber_state_json_writer](auto&& driver_activated) {
      if (auto writer = weak_grabber_state_json_writer.lock()) {
        writer->set_driver_activated(driver_activated);
      }
    });

    virtual_hid_device_service_client_->driver_connected.connect([weak_grabber_state_json_writer](auto&& driver_connected) {
      if (auto writer = weak_grabber_state_json_writer.lock()) {
        writer->set_driver_connected(driver_connected);
      }
    });

    virtual_hid_device_service_client_->driver_version_mismatched.connect([weak_grabber_state_json_writer](auto&& driver_version_mismatched) {
      if (auto writer = weak_grabber_state_json_writer.lock()) {
        writer->set_driver_version_mismatched(driver_version_mismatched);
      }
    });

    virtual_hid_device_service_client_->virtual_hid_keyboard_ready.connect([this](auto&& ready) {
      if (virtual_hid_devices_state_.get_virtual_hid_keyboard_ready() != ready) {
        logger::get_logger()->info("virtual_hid_device_service_client_ virtual_hid_keyboard_ready_response: {0}", ready);

        virtual_hid_devices_state_.set_virtual_hid_keyboard_ready(ready);
        async_post_virtual_hid_devices_state_changed_event();

        // The virtual_hid_keyboard might be terminated due to virtual_hid_device_service_client_ error.
        // We try to reinitialize the device.
        if (!ready) {
          virtual_hid_device_service_client_->async_virtual_hid_keyboard_terminate();
          update_virtual_hid_keyboard();
        }

        update_devices_disabled();
        async_grab_devices();
      }
    });

    virtual_hid_device_service_client_->virtual_hid_pointing_ready.connect([this](auto&& ready) {
      if (virtual_hid_devices_state_.get_virtual_hid_pointing_ready() != ready) {
        logger::get_logger()->info("virtual_hid_device_service_client_ virtual_hid_pointing_ready_response: {0}", ready);

        virtual_hid_devices_state_.set_virtual_hid_pointing_ready(ready);
        async_post_virtual_hid_devices_state_changed_event();

        // The virtual_hid_pointing might be terminated due to virtual_hid_device_service_client_ error.
        // We try to reinitialize the device.
        if (!ready) {
          virtual_hid_device_service_client_->async_virtual_hid_pointing_terminate();
          update_virtual_hid_pointing();
        }

        update_devices_disabled();
        async_grab_devices();
      }
    });

    post_event_to_virtual_devices_manipulator_ =
        std::make_shared<manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices>(
            weak_console_user_server_client,
            notification_message_manager_);
    post_event_to_virtual_devices_manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::manipulators::base>(post_event_to_virtual_devices_manipulator_));

    complex_modifications_applied_event_queue_->enable_manipulator_environment_json_output(constants::get_manipulator_environment_json_file_path());

    // Connect manipulator_managers

    manipulator_managers_connector_.emplace_back_connection(simple_modifications_manipulator_manager_->get_manipulator_manager(),
                                                            merged_input_event_queue_,
                                                            simple_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(complex_modifications_manipulator_manager_,
                                                            complex_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(fn_function_keys_manipulator_manager_->get_manipulator_manager(),
                                                            fn_function_keys_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(post_event_to_virtual_devices_manipulator_manager_,
                                                            posted_event_queue_);

    external_signal_connections_.emplace_back(
        krbn_notification_center::get_instance().input_event_arrived.connect([this] {
          manipulate(pqrs::osx::chrono::mach_absolute_time_point());
        }));

    // hid_manager_

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::pointer),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::joystick),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::game_pad),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                  matching_dictionaries,
                                                                  std::chrono::milliseconds(1000));

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = make_device_id(registry_entry_id);

        //
        // entries_
        //

        auto entry = std::make_shared<device_grabber_details::entry>(device_id,
                                                                     *device_ptr,
                                                                     memory_utility::make_weak(core_configuration_));
        entries_[device_id] = entry;

        entry->hid_queue_values_arrived.connect([this](auto&& entry, auto&& event_queue) {
          values_arrived(entry, event_queue);
        });

        entry->get_hid_queue_value_monitor()->started.connect([this, device_id] {
          auto it = entries_.find(device_id);
          if (it != std::end(entries_)) {
            logger::get_logger()->info("{0} hid queue value monitor is started ({1}).",
                                       it->second->get_device_name(),
                                       it->second->seized() ? "grabbed" : "observed");
            logger_unique_filter_.reset();

            post_device_grabbed_event(it->second->get_device_properties());

            update_caps_lock_led();

            update_virtual_hid_pointing();
          }
        });

        entry->get_hid_queue_value_monitor()->stopped.connect([this, device_id] {
          auto it = entries_.find(device_id);
          if (it != std::end(entries_)) {
            logger::get_logger()->info("{0} hid queue value monitor is stopped.",
                                       it->second->get_device_name());
            logger_unique_filter_.reset();

            post_device_ungrabbed_event(device_id);

            update_virtual_hid_pointing();
          }
        });

        entry->get_hid_queue_value_monitor()->error_occurred.connect([](auto&& message, auto&& kr) {
          if (kr.not_permitted()) {
            logger::get_logger()->warn("hid_queue_value_monitor not_permitted error");
            if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
              killer->async_kill();
            }
          }
        });

        // ----------------------------------------

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

      // entries_

      {
        auto it = entries_.find(device_id);
        if (it != std::end(entries_)) {
          logger::get_logger()->info("{0} is terminated.",
                                     it->second->get_device_name());
          logger_unique_filter_.reset();

          auto device_properties = it->second->get_device_properties();
          if (device_properties.get_is_keyboard().value_or(false) &&
              device_properties.get_is_karabiner_virtual_hid_device().value_or(false)) {
            virtual_hid_device_service_client_->async_stop();
            async_ungrab_devices();

            virtual_hid_device_service_client_->async_start();
          }

          entries_.erase(it);
        }
      }

      //
      // Unregister device
      //

      if (notification_message_manager_) {
        notification_message_manager_->async_erase_device(device_id);
      }

      hat_switch_converter::get_global_hat_switch_converter()->erase_device(device_id);

      // ----------------------------------------

      output_devices_json();
      output_device_details_json();

      // ----------------------------------------

      post_device_ungrabbed_event(device_id);

      update_virtual_hid_pointing();

      // ----------------------------------------
      update_devices_disabled();
      async_grab_devices();
    });

    hid_manager_->error_occurred.connect([this](auto&& message, auto&& kern_return) {
      logger::get_logger()->error("{0}: {1}", message, kern_return.to_string());
      logger_unique_filter_.reset();
    });

    //
    // power_management_monitor_
    //

    power_management_monitor_ = std::make_unique<pqrs::osx::iokit_power_management::monitor>(weak_dispatcher_,
                                                                                             run_loop_thread_utility::get_power_management_run_loop_thread());

    power_management_monitor_->system_will_sleep.connect([this](auto&& kernel_port,
                                                                auto&& notification_id,
                                                                auto&& wait) {
      logger::get_logger()->info("system_will_sleep");

      set_system_sleeping(true);

      enqueue_to_dispatcher(
          [kernel_port, notification_id, wait]() {
            logger::get_logger()->info("call IOAllowPowerChange");

            IOAllowPowerChange(kernel_port, notification_id);

            wait->notify();
          },
          when_now() + std::chrono::seconds(1));
    });

    power_management_monitor_->system_will_power_on.connect([this] {
      logger::get_logger()->info("system_will_power_on");

      set_system_sleeping(false);
    });

    power_management_monitor_->system_has_powered_on.connect([this] {
      logger::get_logger()->info("system_has_powered_on");

      set_system_sleeping(false);
    });

    power_management_monitor_->can_system_sleep.connect([](auto&& kernel_port,
                                                           auto&& notification_id,
                                                           auto&& wait) {
      logger::get_logger()->info("can_system_sleep");

      IOAllowPowerChange(kernel_port, notification_id);

      wait->notify();
    });

    power_management_monitor_->system_will_not_sleep.connect([this] {
      logger::get_logger()->info("system_will_not_sleep");

      set_system_sleeping(false);
    });

    power_management_monitor_->error_occurred.connect([](auto&& message) {
      logger::get_logger()->error("power_management_monitor_ error: {0}", message);
    });

    power_management_monitor_->async_start();
  }

  virtual ~device_grabber(void) {
    detach_from_dispatcher([this] {
      stop();

      power_management_monitor_ = nullptr;

      hid_manager_ = nullptr;

      external_signal_connections_.clear();

      post_event_to_virtual_devices_manipulator_ = nullptr;

      simple_modifications_manipulator_manager_ = nullptr;
      complex_modifications_manipulator_manager_ = nullptr;
      fn_function_keys_manipulator_manager_ = nullptr;
      post_event_to_virtual_devices_manipulator_manager_ = nullptr;
      virtual_hid_device_service_client_ = nullptr;

      notification_message_manager_ = nullptr;

      hat_switch_converter::get_global_hat_switch_converter()->clear();
    });
  }

  void async_start(const std::string& user_core_configuration_file_path,
                   uid_t expected_user_core_configuration_file_owner) {
    enqueue_to_dispatcher([this,
                           user_core_configuration_file_path,
                           expected_user_core_configuration_file_owner] {
      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_monitor here.
      event_tap_monitor_ = std::make_unique<event_tap_monitor>();

      event_tap_monitor_->pointing_device_event_arrived.connect([this](auto&& event_type, auto&& event) {
        auto e = event_queue::event::make_pointing_device_event_from_event_tap_event();
        event_queue::entry entry(device_id(0),
                                 event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                                 e,
                                 event_type,
                                 event,
                                 event_queue::state::virtual_event);

        merged_input_event_queue_->push_back_entry(entry);

        krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
      });

      event_tap_monitor_->async_start();

      configuration_monitor_ = std::make_unique<configuration_monitor>(user_core_configuration_file_path,
                                                                       expected_user_core_configuration_file_owner);

      configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
        if (auto core_configuration = weak_core_configuration.lock()) {
          core_configuration_ = core_configuration;

          for (auto&& e : entries_) {
            e.second->set_weak_core_configuration(core_configuration);
          }

          manipulator_managers_connector_.set_manipulator_environment_core_configuration(core_configuration);

          logger_unique_filter_.reset();

          //
          // Reflect profile changes
          //

          auto& profile = core_configuration_->get_selected_profile();

          if (hid_manager_) {
            hid_manager_->async_set_device_matched_delay(
                profile.get_parameters().get_delay_milliseconds_before_open_device());
            hid_manager_->async_start();
          }

          simple_modifications_manipulator_manager_->update(profile);
          fn_function_keys_manipulator_manager_->update(profile,
                                                        system_preferences_properties_);
          update_complex_modifications_manipulators();

          update_virtual_hid_keyboard();
          update_virtual_hid_pointing();

          update_devices_disabled();
          async_grab_devices();
          async_post_system_preferences_properties_changed_event();
        }
      });

      configuration_monitor_->async_start();

      virtual_hid_device_service_client_->async_start();
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
        grab_device(*(e.second));
      }
    });
  }

  void async_ungrab_devices(void) {
    enqueue_to_dispatcher([this] {
      for (auto&& e : entries_) {
        e.second->get_hid_queue_value_monitor()->async_stop();
      }

      logger::get_logger()->info("Connected devices are ungrabbed");
    });
  }

  void async_set_system_preferences_properties(const pqrs::osx::system_preferences::properties& value) {
    enqueue_to_dispatcher([this, value] {
      system_preferences_properties_ = value;

      fn_function_keys_manipulator_manager_->update(core_configuration_->get_selected_profile(),
                                                    system_preferences_properties_);
      async_post_system_preferences_properties_changed_event();
    });
  }

  void async_post_set_variable_event(const manipulator_environment_variable_set_variable& value) {
    enqueue_to_dispatcher([this, value] {
      auto event = event_queue::event::make_set_variable_event(value);

      for (const auto& t : {event_type::key_down, event_type::key_up}) {
        event_queue::entry entry(device_id(0),
                                 event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                                 event,
                                 t,
                                 event,
                                 event_queue::state::virtual_event);
        merged_input_event_queue_->push_back_entry(entry);
      }

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_frontmost_application_changed_event(const pqrs::osx::frontmost_application_monitor::application& application) {
    enqueue_to_dispatcher([this, application] {
      auto event = event_queue::event::make_frontmost_application_changed_event(application);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event,
                               event_queue::state::virtual_event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_input_source_changed_event(const pqrs::osx::input_source::properties& properties) {
    enqueue_to_dispatcher([this, properties] {
      auto event = event_queue::event::make_input_source_changed_event(properties);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event,
                               event_queue::state::virtual_event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_system_preferences_properties_changed_event(void) {
    enqueue_to_dispatcher([this] {
      auto event = event_queue::event::make_system_preferences_properties_changed_event(
          system_preferences_properties_);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event,
                               event_queue::state::virtual_event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

  void async_post_virtual_hid_devices_state_changed_event(void) {
    enqueue_to_dispatcher([this] {
      auto event = event_queue::event::make_virtual_hid_devices_state_changed_event(virtual_hid_devices_state_);
      event_queue::entry entry(device_id(0),
                               event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                               event,
                               event_type::single,
                               event,
                               event_queue::state::virtual_event);

      merged_input_event_queue_->push_back_entry(entry);

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    });
  }

private:
  void stop(void) {
    configuration_monitor_ = nullptr;

    async_ungrab_devices();

    event_tap_monitor_ = nullptr;

    virtual_hid_device_service_client_->async_stop();
  }

  // This method is executed in the shared dispatcher thread.
  void manipulate(absolute_time_point now) {
    manipulator_managers_connector_.manipulate(now);

    posted_event_queue_->clear_events();
    post_event_to_virtual_devices_manipulator_->async_post_events(virtual_hid_device_service_client_);

    if (auto min = manipulator_managers_connector_.min_input_event_time_stamp()) {
      auto when = when_now();
      if (now < *min) {
        when += pqrs::osx::chrono::make_milliseconds(*min - now);
      }

      enqueue_to_dispatcher(
          [this, min] {
            manipulate(*min);
          },
          when);
    }
  }

  // This method is executed in the shared dispatcher thread.
  void values_arrived(device_grabber_details::entry& entry,
                      std::shared_ptr<event_queue::queue> event_queue) {
    if (entry.is_karabiner_virtual_hid_device()) {
      // Handle caps_lock_state_changed event only if the hid is Karabiner-DriverKit-VirtualHIDDevice.
      for (const auto& e : event_queue->get_entries()) {
        if (e.get_event().get_type() == event_queue::event::type::caps_lock_state_changed) {
          if (auto state = e.get_event().get_integer_value()) {
            last_caps_lock_state_ = *state;
            post_caps_lock_state_changed_event(*state);
            update_caps_lock_led();
          }
        }
      }
    } else {
      // Manipulate events

      bool needs_regrab = false;
      bool notify = false;

      for (const auto& e : event_queue->get_entries()) {
        if (auto ev = e.get_event().get_if<momentary_switch_event>()) {
          needs_regrab |= entry.get_probable_stuck_events_manager()->update(
              *ev,
              e.get_event_type(),
              entry.seized() ? device_state::seized
                             : device_state::observed);
        }

        if (!entry.get_disabled() && entry.seized()) {
          event_queue::entry qe(e.get_device_id(),
                                e.get_event_time_stamp(),
                                e.get_event(),
                                e.get_event_type(),
                                e.get_original_event(),
                                e.get_state());

          merged_input_event_queue_->push_back_entry(qe);

          notify = true;
        }
      }

      if (needs_regrab) {
        grab_device(entry);
      }

      if (notify) {
        krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
      }
    }
  }

  void post_device_grabbed_event(const device_properties& device_properties) {
    auto event = event_queue::event::make_device_grabbed_event(device_properties);
    event_queue::entry entry(device_properties.get_device_id(),
                             event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                             event,
                             event_type::single,
                             event,
                             event_queue::state::virtual_event);

    merged_input_event_queue_->push_back_entry(entry);

    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
  }

  void post_device_ungrabbed_event(device_id device_id) {
    auto event = event_queue::event::make_device_ungrabbed_event();
    event_queue::entry entry(device_id,
                             event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                             event,
                             event_type::single,
                             event,
                             event_queue::state::virtual_event);

    merged_input_event_queue_->push_back_entry(entry);

    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
  }

  void post_caps_lock_state_changed_event(bool caps_lock_state) {
    auto event = event_queue::event::make_caps_lock_state_changed_event(caps_lock_state);
    event_queue::entry entry(device_id(0),
                             event_queue::event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point()),
                             event,
                             event_type::single,
                             event,
                             event_queue::state::virtual_event);

    merged_input_event_queue_->push_back_entry(entry);

    krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
  }

  // This method is executed in the shared dispatcher thread.
  void grab_device(device_grabber_details::entry& entry) const {
    auto state = make_grabbable_state(entry);
    switch (state) {
      case grabbable_state::state::grabbable:
      case grabbable_state::state::ungrabbable_temporarily:
        entry.async_start_queue_value_monitor(state);
        break;

      case grabbable_state::state::ungrabbable_permanently:
      case grabbable_state::state::none:
      case grabbable_state::state::end_:
        entry.async_stop_queue_value_monitor();
        break;
    }
  }

  // This method is executed in the shared dispatcher thread.
  void grab_device(device_id device_id) const {
    auto it = entries_.find(device_id);
    if (it != std::end(entries_)) {
      grab_device(*(it->second));
    }
  }

  // This method is executed in the shared dispatcher thread.
  grabbable_state::state make_grabbable_state(const device_grabber_details::entry& entry) const {
    // The device is always grabbable if it is observed device
    // because karabiner_grabber does not seize the device and do not affect existing hidd processing.
    // (e.g. key repeat)

    if (entry.needs_to_observe_device()) {
      return grabbable_state::state::grabbable;
    }

    if (!entry.needs_to_seize_device()) {
      return grabbable_state::state::ungrabbable_permanently;
    }

    //
    // In macOS, the behavior of devices in sleep differs depending on whether the device is seized or not.
    // For devices that have been seized, it will attempt to wake up on any event.
    // In other words, even moving the mouse pointer will prevent sleep.
    //
    // There seems to be no way to avoid this behavior, at least on macOS 13, other than to ungrab the device.
    // Therefore, do not grab the device while system is sleeping.
    //

    if (system_sleeping_) {
      return grabbable_state::state::ungrabbable_temporarily;
    }

    //
    // Ungrabbable while virtual_hid_device_service_client_ is not ready.
    //

    if (!virtual_hid_devices_state_.get_virtual_hid_keyboard_ready()) {
      std::string message = "virtual_hid_keyboard is not ready. Please wait for a while.";
      logger_unique_filter_.warn(message);
      unset_device_ungrabbable_temporarily_notification_message(entry.get_device_id());
      return grabbable_state::state::ungrabbable_temporarily;
    }

    if (needs_prepare_virtual_hid_pointing_device()) {
      if (!virtual_hid_devices_state_.get_virtual_hid_pointing_ready()) {
        std::string message = "virtual_hid_pointing is not ready. Please wait for a while.";
        logger_unique_filter_.warn(message);
        unset_device_ungrabbable_temporarily_notification_message(entry.get_device_id());
        return grabbable_state::state::ungrabbable_temporarily;
      }
    }

    //
    // Ungrabbable while probable stuck events exist
    //

    if (auto event = entry.get_probable_stuck_events_manager()->find_probable_stuck_event()) {
      auto message = fmt::format("Probable stuck key detected! "
                                 "{0} is ignored temporarily until {1} is pressed again. "
                                 "Key may have been held when keyboard was grabbed. "
                                 "Is the keyboard reconnecting while in use?",
                                 entry.get_device_name(),
                                 nlohmann::json(*event).dump());
      logger_unique_filter_.warn(message);

      if (notification_message_manager_) {
        notification_message_manager_->async_set_device_ungrabbable_temporarily_message(
            entry.get_device_id(),
            fmt::format("Probable stuck key detected!\n\n"
                        "{0} is ignored temporarily until {1} is pressed again.\n\n"
                        "Key may have been held when keyboard was grabbed. "
                        "Is the keyboard reconnecting while in use?",
                        entry.get_device_short_name(),
                        nlohmann::json(*event).dump()));
      }

      return grabbable_state::state::ungrabbable_temporarily;
    }

    // ----------------------------------------

    unset_device_ungrabbable_temporarily_notification_message(entry.get_device_id());

    return grabbable_state::state::grabbable;
  }

  void unset_device_ungrabbable_temporarily_notification_message(device_id id) const {
    if (notification_message_manager_) {
      notification_message_manager_->async_set_device_ungrabbable_temporarily_message(id, "");
    }
  }

  void update_caps_lock_led(void) {
    std::optional<led_state> state;
    if (last_caps_lock_state_) {
      state = *last_caps_lock_state_ ? led_state::on : led_state::off;
    }

    for (auto&& e : entries_) {
      e.second->set_caps_lock_led_state(state);
    }
  }

  bool needs_prepare_virtual_hid_pointing_device(void) const {
    //
    // Check if there is a pointing device to grab
    // (The game pad also sends mouse events, so a virtual pointing device should be prepared as well.)
    //

    for (const auto& e : entries_) {
      if (e.second->needs_to_seize_device()) {
        auto device_properties = e.second->get_device_properties();
        if (device_properties.get_is_pointing_device().value_or(false) ||
            device_properties.get_is_game_pad().value_or(false)) {
          return true;
        }
      }
    }

    //
    // Check if a setting exists that would fire pointing device events
    //

    if (manipulator_managers_connector_.needs_virtual_hid_pointing()) {
      return true;
    }

    return false;
  }

  void update_virtual_hid_keyboard(void) {
    virtual_hid_device_service_client_->async_virtual_hid_keyboard_initialize(
        core_configuration_->get_selected_profile().get_virtual_hid_keyboard().get_country_code());
  }

  void update_virtual_hid_pointing(void) {
    if (needs_prepare_virtual_hid_pointing_device()) {
      virtual_hid_device_service_client_->async_virtual_hid_pointing_initialize();
      return;
    }

    virtual_hid_device_service_client_->async_virtual_hid_pointing_terminate();
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
      if (e.second->determine_is_built_in_keyboard() &&
          need_to_disable_built_in_keyboard()) {
        e.second->set_disabled(true);
      } else {
        e.second->set_disabled(false);
      }
    }
  }

  void output_devices_json(void) const {
    connected_devices::connected_devices connected_devices;
    for (const auto& e : entries_) {
      connected_devices::details::device d(e.second->get_device_properties());
      connected_devices.push_back_device(d);
    }

    auto file_path = constants::get_devices_json_file_path();
    connected_devices.async_save_to_file(file_path);
  }

  void output_device_details_json(void) const {
    std::vector<device_properties> device_details;
    for (const auto& e : entries_) {
      device_details.push_back(e.second->get_device_properties());
    }

    std::sort(std::begin(device_details),
              std::end(device_details),
              [](auto& a, auto& b) {
                return a.compare(b);
              });

    auto file_path = constants::get_device_details_json_file_path();
    json_writer::async_save_to_file(nlohmann::json(device_details), file_path, 0755, 0644);
  }

  void update_complex_modifications_manipulators(void) {
    complex_modifications_manipulator_manager_->invalidate_manipulators();

    for (const auto& rule : core_configuration_->get_selected_profile().get_complex_modifications().get_rules()) {
      for (const auto& manipulator : rule.get_manipulators()) {
        try {
          auto m = manipulator::manipulator_factory::make_manipulator(manipulator.get_json(),
                                                                      manipulator.get_parameters());
          for (const auto& c : manipulator.get_conditions()) {
            m->push_back_condition(manipulator::manipulator_factory::make_condition(c.get_json()));
          }
          complex_modifications_manipulator_manager_->push_back_manipulator(m);

        } catch (const pqrs::json::unmarshal_error& e) {
          logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }
    }
  }

  void set_system_sleeping(bool value) {
    system_sleeping_ = value;

    update_devices_disabled();
    async_grab_devices();
  }

  std::shared_ptr<pqrs::karabiner::driverkit::virtual_hid_device_service::client> virtual_hid_device_service_client_;

  virtual_hid_devices_state virtual_hid_devices_state_;

  std::vector<nod::scoped_connection> external_signal_connections_;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration_;

  std::unique_ptr<event_tap_monitor> event_tap_monitor_;
  std::optional<bool> last_caps_lock_state_;
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<device_id, std::shared_ptr<device_grabber_details::entry>> entries_;

  std::unique_ptr<pqrs::osx::iokit_power_management::monitor> power_management_monitor_;
  bool system_sleeping_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;

  std::shared_ptr<notification_message_manager> notification_message_manager_;

  std::shared_ptr<event_queue::queue> merged_input_event_queue_;

  std::shared_ptr<device_grabber_details::simple_modifications_manipulator_manager> simple_modifications_manipulator_manager_;
  std::shared_ptr<event_queue::queue> simple_modifications_applied_event_queue_;

  std::shared_ptr<manipulator::manipulator_manager> complex_modifications_manipulator_manager_;
  std::shared_ptr<event_queue::queue> complex_modifications_applied_event_queue_;

  std::shared_ptr<device_grabber_details::fn_function_keys_manipulator_manager> fn_function_keys_manipulator_manager_;
  std::shared_ptr<event_queue::queue> fn_function_keys_applied_event_queue_;

  std::shared_ptr<manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator_;
  std::shared_ptr<manipulator::manipulator_manager> post_event_to_virtual_devices_manipulator_manager_;
  std::shared_ptr<event_queue::queue> posted_event_queue_;

  mutable pqrs::spdlog::unique_filter logger_unique_filter_;
};
} // namespace grabber
} // namespace krbn
