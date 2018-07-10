#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "device_detail.hpp"
#include "event_tap_manager.hpp"
#include "gcd_utility.hpp"
#include "hid_manager.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "json_utility.hpp"
#include "krbn_notification_center.hpp"
#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
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
class device_grabber final {
public:
  enum class mode {
    observing,
    grabbing,
  };

  device_grabber(const device_grabber&) = delete;

  device_grabber(void) : profile_(nlohmann::json()),
                         merged_input_event_queue_(std::make_shared<event_queue>()),
                         simple_modifications_applied_event_queue_(std::make_shared<event_queue>()),
                         complex_modifications_applied_event_queue_(std::make_shared<event_queue>()),
                         fn_function_keys_applied_event_queue_(std::make_shared<event_queue>()),
                         posted_event_queue_(std::make_shared<event_queue>()),
                         mode_(mode::observing),
                         suspended_(false) {
    client_connected_connection = virtual_hid_device_client_.client_connected.connect([this]() {
      logger::get_logger().info("virtual_hid_device_client_ is connected");

      update_virtual_hid_keyboard();
      update_virtual_hid_pointing();
    });

    client_disconnected_connection = virtual_hid_device_client_.client_disconnected.connect([this]() {
      logger::get_logger().info("virtual_hid_device_client_ is disconnected");

      stop_grabbing();
    });

    post_event_to_virtual_devices_manipulator_ = std::make_shared<manipulator::details::post_event_to_virtual_devices>(system_preferences_);
    post_event_to_virtual_devices_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(post_event_to_virtual_devices_manipulator_));

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

    input_event_arrived_connection_ = krbn_notification_center::get_instance().input_event_arrived.connect([this]() {
      manipulate(mach_absolute_time());
    });

    manipulator_timer_invoked_connection_ = manipulator::manipulator_timer::get_instance().timer_invoked.connect([this](auto timer_id, auto now) {
      if (manipulator_timer_id_ == timer_id) {
        manipulator_timer_id_ = boost::none;
        manipulate(now);
      }
    });

    // macOS 10.12 sometimes synchronize caps lock LED to internal keyboard caps lock state.
    // The behavior causes LED state mismatch because device_grabber does not change the caps lock state of physical keyboards.
    // Thus, we monitor the LED state and update it if needed.
    led_monitor_timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
        1.0 * NSEC_PER_SEC,
        0,
        ^{
          if (event_tap_manager_) {
            if (auto state = event_tap_manager_->get_caps_lock_state()) {
              update_caps_lock_led(*state);
            }
          }
        });

    hid_manager_.device_detecting.connect([](auto&& device) {
      if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
        return false;
      }
      return true;
    });

    hid_manager_.device_detected.connect([this](auto&& human_interface_device) {
      human_interface_device.set_is_grabbable_callback(std::bind(&device_grabber::is_grabbable_callback, this, std::placeholders::_1));
      human_interface_device.device_grabbed.connect([this](auto&& human_interface_device) {
        logger::get_logger().info("{0} is grabbed", human_interface_device.get_name_for_log());
        device_grabbed(human_interface_device);
      });
      human_interface_device.device_ungrabbed.connect([this](auto&& human_interface_device) {
        logger::get_logger().info("{0} is ungrabbed", human_interface_device.get_name_for_log());
        device_ungrabbed(human_interface_device);
      });
      human_interface_device.device_disabled.connect([this](auto&& human_interface_device) {
        device_disabled(human_interface_device);
      });
      human_interface_device.values_arrived.connect([this](auto&& human_interface_device,
                                                           auto&& event_queue) {
        values_arrived(human_interface_device, event_queue);
      });

      output_devices_json();
      output_device_details_json();

      update_virtual_hid_pointing();

      // ----------------------------------------
      if (mode_ == mode::grabbing) {
        grab_devices();
      }
    });

    hid_manager_.device_removed.connect([this](auto&& human_interface_device) {
      human_interface_device.ungrab();

      grabbable_states_.erase(human_interface_device.get_registry_entry_id());
      first_grabbed_event_time_stamps_.erase(human_interface_device.get_registry_entry_id());

      output_devices_json();
      output_device_details_json();

      // ----------------------------------------

      if (human_interface_device.is_keyboard() &&
          human_interface_device.is_karabiner_virtual_hid_device()) {
        virtual_hid_device_client_.close();
        ungrab_devices();

        virtual_hid_device_client_.connect();
        grab_devices();
      }

      update_virtual_hid_pointing();

      // ----------------------------------------
      // Refresh grab state in order to apply disable_built_in_keyboard_if_exists.

      if (mode_ == mode::grabbing) {
        grab_devices();
      }
    });

    hid_manager_.start({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });
  }

  ~device_grabber(void) {
    // Release manager_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      stop_grabbing();

      hid_manager_.stop();

      input_event_arrived_connection_.disconnect();
      manipulator_timer_invoked_connection_.disconnect();

      led_monitor_timer_ = nullptr;

      client_connected_connection.disconnect();
      client_disconnected_connection.disconnect();
    });
  }

  void update_grabbable_state(registry_entry_id registry_entry_id,
                              grabbable_state grabbable_state,
                              ungrabbable_temporarily_reason ungrabbable_temporarily_reason,
                              uint64_t time_stamp) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      // Ignore if the first grabbed event is already arrived.
      {
        auto it = first_grabbed_event_time_stamps_.find(registry_entry_id);
        if (it != std::end(first_grabbed_event_time_stamps_) &&
            it->second <= time_stamp) {
          return;
        }
      }

      {
        auto it = grabbable_states_.find(registry_entry_id);
        if (it == std::end(grabbable_states_)) {
          grabbable_states_[registry_entry_id] = {};
        }
      }

      auto& entries = grabbable_states_[registry_entry_id];

      entries.push_back(
          std::make_shared<grabbable_state_entry>(grabbable_state,
                                                  ungrabbable_temporarily_reason,
                                                  time_stamp));

      // Keep multiple entries for when the first grabbed event(s) calls
      // `update_grabbable_state` before `values_arrived` callback.

      const int max_entries = 32;

      while (entries.size() > max_entries) {
        grabbable_states_[registry_entry_id].pop_front();
      }

      if (grabbable_state != grabbable_state::grabbable) {
        if (auto hid = hid_manager_.find_human_interface_device(registry_entry_id)) {
          if (hid->get_grabbed()) {
            hid->ungrab();
            hid->grab();
          }
        }
      }
    });
  }

  void start_grabbing(const std::string& user_core_configuration_file_path) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      mode_ = mode::grabbing;

      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_manager here.
      event_tap_manager_ = std::make_unique<event_tap_manager>(std::bind(&device_grabber::caps_lock_state_changed_callback,
                                                                         this,
                                                                         std::placeholders::_1),
                                                               std::bind(&device_grabber::event_tap_pointing_device_event_callback,
                                                                         this,
                                                                         std::placeholders::_1,
                                                                         std::placeholders::_2));

      configuration_monitor_ = std::make_unique<configuration_monitor>(user_core_configuration_file_path,
                                                                       [this](std::shared_ptr<core_configuration> core_configuration) {
                                                                         core_configuration_ = core_configuration;

                                                                         is_grabbable_callback_log_reducer_.reset();
                                                                         set_profile(core_configuration_->get_selected_profile());
                                                                         grab_devices();
                                                                       });

      virtual_hid_device_client_.connect();
    });
  }

  void stop_grabbing(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      configuration_monitor_ = nullptr;

      ungrab_devices();

      mode_ = mode::observing;

      event_tap_manager_ = nullptr;

      virtual_hid_device_client_.close();
    });
  }

  void grab_devices(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (const auto& it : hid_manager_.get_hids()) {
        if ((it.second)->is_grabbable() == grabbable_state::ungrabbable_permanently) {
          (it.second)->ungrab();
        } else {
          (it.second)->grab();
        }
      }

      enable_devices();
    });
  }

  void ungrab_devices(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (auto&& it : hid_manager_.get_hids()) {
        (it.second)->ungrab();
      }

      logger::get_logger().info("Connected devices are ungrabbed");
    });
  }

  void suspend(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (!suspended_) {
        logger::get_logger().info("device_grabber::suspend");

        suspended_ = true;

        if (mode_ == mode::grabbing) {
          ungrab_devices();
        }
      }
    });
  }

  void resume(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (suspended_) {
        logger::get_logger().info("device_grabber::resume");

        suspended_ = false;

        if (mode_ == mode::grabbing) {
          grab_devices();
        }
      }
    });
  }

  void unset_profile(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      profile_ = core_configuration::profile(nlohmann::json());

      manipulator_managers_connector_.invalidate_manipulators();
    });
  }

  void set_system_preferences(const system_preferences& value) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      system_preferences_ = value;

      update_fn_function_keys_manipulators();
      post_keyboard_type_changed_event();
    });
  }

  void post_frontmost_application_changed_event(const std::string& bundle_identifier,
                                                const std::string& file_path) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      auto event = event_queue::queued_event::event::make_frontmost_application_changed_event(bundle_identifier,
                                                                                              file_path);
      event_queue::queued_event queued_event(device_id(0),
                                             event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                             event,
                                             event_type::single,
                                             event);

      merged_input_event_queue_->push_back_event(queued_event);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

  void post_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      auto event = event_queue::queued_event::event::make_input_source_changed_event(input_source_identifiers);
      event_queue::queued_event queued_event(device_id(0),
                                             event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                             event,
                                             event_type::single,
                                             event);

      merged_input_event_queue_->push_back_event(queued_event);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

  void post_keyboard_type_changed_event(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      auto keyboard_type_string = system_preferences_utility::get_keyboard_type_string(system_preferences_.get_keyboard_type());
      auto event = event_queue::queued_event::event::make_keyboard_type_changed_event(keyboard_type_string);
      event_queue::queued_event queued_event(device_id(0),
                                             event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                             event,
                                             event_type::single,
                                             event);

      merged_input_event_queue_->push_back_event(queued_event);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

private:
  class grabbable_state_entry {
  public:
    grabbable_state_entry(grabbable_state grabbable_state,
                          ungrabbable_temporarily_reason ungrabbable_temporarily_reason,
                          uint64_t time_stamp) : grabbable_state_(grabbable_state),
                                                 ungrabbable_temporarily_reason_(ungrabbable_temporarily_reason),
                                                 time_stamp_(time_stamp),
                                                 logged_(false) {
    }

    grabbable_state get_grabbable_state(void) const {
      return grabbable_state_;
    }

    ungrabbable_temporarily_reason get_ungrabbable_temporarily_reason(void) const {
      return ungrabbable_temporarily_reason_;
    }

    uint64_t get_time_stamp(void) const {
      return time_stamp_;
    }

    bool get_logged(void) const {
      return logged_;
    }

    void set_logged(void) {
      logged_ = true;
    }

  private:
    grabbable_state grabbable_state_;
    ungrabbable_temporarily_reason ungrabbable_temporarily_reason_;
    uint64_t time_stamp_;
    bool logged_;
  };

  void manipulate(uint64_t now) {
    {
      // Avoid recursive call
      std::unique_lock<std::mutex> lock(manipulate_mutex_, std::try_to_lock);

      if (lock.owns_lock()) {
        manipulator_managers_connector_.manipulate(now);

        posted_event_queue_->clear_events();
        post_event_to_virtual_devices_manipulator_->post_events(virtual_hid_device_client_);
      }
    }

    if (auto min = manipulator_managers_connector_.min_input_event_time_stamp()) {
      manipulator_timer_id_ = manipulator::manipulator_timer::get_instance().add_entry(*min);
    }
  }

  void values_arrived(human_interface_device& device,
                      event_queue& event_queue) {
    // Update first_grabbed_event_time_stamps_

    for (const auto& queued_event : event_queue.get_events()) {
      if (auto device_detail = types::find_device_detail(queued_event.get_device_id())) {
        if (auto registry_entry_id = device_detail->get_registry_entry_id()) {
          auto it = first_grabbed_event_time_stamps_.find(*registry_entry_id);
          if (it == std::end(first_grabbed_event_time_stamps_)) {
            auto time_stamp = queued_event.get_event_time_stamp().get_time_stamp();
            first_grabbed_event_time_stamps_.emplace(*registry_entry_id, time_stamp);

            logger::get_logger().info("first grabbed event: registry_entry_id:{0} time_stamp:{1}",
                                      static_cast<uint64_t>(*registry_entry_id),
                                      time_stamp);

            // Update grabbable_states_
            erase_grabbable_state_entries_after_first_grabbed_event(*registry_entry_id,
                                                                    time_stamp);
          }
        }
      }
    }

    // Manipulate events

    if (device.get_disabled()) {
      // Do nothing
    } else {
      for (const auto& queued_event : event_queue.get_events()) {
        event_queue::queued_event qe(queued_event.get_device_id(),
                                     queued_event.get_event_time_stamp(),
                                     queued_event.get_event(),
                                     queued_event.get_event_type(),
                                     queued_event.get_original_event());

        merged_input_event_queue_->push_back_event(qe);
      }
    }

    krbn_notification_center::get_instance().input_event_arrived();
    // manipulator_managers_connector_.log_events_sizes(logger::get_logger());
  }

  void erase_grabbable_state_entries_after_first_grabbed_event(registry_entry_id registry_entry_id,
                                                               uint64_t time_stamp) {
    auto it = grabbable_states_.find(registry_entry_id);
    if (it != std::end(grabbable_states_)) {
      auto& entries = it->second;
      entries.erase(std::remove_if(std::begin(entries),
                                   std::end(entries),
                                   [&](const auto& e) {
                                     return e->get_time_stamp() >= time_stamp;
                                   }),
                    std::end(entries));
    }
  }

  void post_device_ungrabbed_event(device_id device_id) {
    auto event = event_queue::queued_event::event::make_device_ungrabbed_event();
    event_queue::queued_event queued_event(device_id,
                                           event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                           event,
                                           event_type::single,
                                           event);

    merged_input_event_queue_->push_back_event(queued_event);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  void post_caps_lock_state_changed_callback(bool caps_lock_state) {
    event_queue::queued_event::event event(event_queue::queued_event::event::type::caps_lock_state_changed, caps_lock_state);
    event_queue::queued_event queued_event(device_id(0),
                                           event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                           event,
                                           event_type::single,
                                           event);

    merged_input_event_queue_->push_back_event(queued_event);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  grabbable_state is_grabbable_callback(human_interface_device& device) {
    if (is_ignored_device(device)) {
      // If we need to disable the built-in keyboard, we have to grab it.
      if (device.is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        // Do nothing
      } else {
        logger::get_logger().info("{0} is ignored.", device.get_name_for_log());
        return grabbable_state::ungrabbable_permanently;
      }
    }

    // ----------------------------------------
    // Ungrabbable while virtual_hid_device_client_ is not ready.

    if (!virtual_hid_device_client_.is_connected()) {
      std::string message = "virtual_hid_device_client is not connected yet. Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return grabbable_state::ungrabbable_temporarily;
    }

    if (!virtual_hid_device_client_.is_virtual_hid_keyboard_ready()) {
      std::string message = "virtual_hid_keyboard is not ready. Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Check observer state

    {
      auto it = grabbable_states_.find(device.get_registry_entry_id());
      if (it == std::end(grabbable_states_)) {
        std::string message = fmt::format("{0} is not observed yet. Please wait for a while.",
                                          device.get_name_for_log());
        is_grabbable_callback_log_reducer_.warn(message);
        return grabbable_state::ungrabbable_temporarily;
      }

      if (it->second.empty()) {
        std::string message = fmt::format("{0} is ungrabbable temporarily.",
                                          device.get_name_for_log());
        is_grabbable_callback_log_reducer_.warn(message);
        return grabbable_state::ungrabbable_temporarily;
      }

      switch (it->second.back()->get_grabbable_state()) {
        case grabbable_state::grabbable:
          break;

        case grabbable_state::ungrabbable_temporarily: {
          std::string message;
          switch (it->second.back()->get_ungrabbable_temporarily_reason()) {
            case ungrabbable_temporarily_reason::none: {
              message = fmt::format("{0} is ungrabbable temporarily",
                                    device.get_name_for_log());
              break;
            }
            case ungrabbable_temporarily_reason::key_repeating: {
              message = fmt::format("{0} is ungrabbable temporarily while a key is repeating.",
                                    device.get_name_for_log());
              break;
            }
            case ungrabbable_temporarily_reason::modifier_key_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while any modifier flags are pressed.",
                                    device.get_name_for_log());
              break;
            }
            case ungrabbable_temporarily_reason::pointing_button_pressed: {
              message = fmt::format("{0} is ungrabbable temporarily while mouse buttons are pressed.",
                                    device.get_name_for_log());
              break;
            }
          }
          is_grabbable_callback_log_reducer_.warn(message);

          return grabbable_state::ungrabbable_temporarily;
        }

        case grabbable_state::ungrabbable_permanently: {
          std::string message = fmt::format("{0} is ungrabbable permanently.",
                                            device.get_name_for_log());
          is_grabbable_callback_log_reducer_.warn(message);
          return grabbable_state::ungrabbable_permanently;
        }

        case grabbable_state::device_error: {
          std::string message = fmt::format("{0} is ungrabbable temporarily by a failure of accessing device.",
                                            device.get_name_for_log());
          is_grabbable_callback_log_reducer_.warn(message);
          return grabbable_state::ungrabbable_temporarily;
        }
      }
    }

    // ----------------------------------------

    return grabbable_state::grabbable;
  }

  void device_grabbed(human_interface_device& device) {
    // set keyboard led
    if (event_tap_manager_) {
      bool state = false;
      if (auto s = event_tap_manager_->get_caps_lock_state()) {
        state = *s;
      }
      update_caps_lock_led(state);
    }

    update_virtual_hid_pointing();

    apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
  }

  void device_ungrabbed(human_interface_device& device) {
    first_grabbed_event_time_stamps_.erase(device.get_registry_entry_id());

    post_device_ungrabbed_event(device.get_device_id());

    update_virtual_hid_pointing();

    apple_notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_device_grabbing_state_is_changed());
  }

  void device_disabled(human_interface_device& device) {
    // Post device_ungrabbed event in order to release modifier_flags.
    post_device_ungrabbed_event(device.get_device_id());
  }

  void caps_lock_state_changed_callback(bool caps_lock_state) {
    post_caps_lock_state_changed_callback(caps_lock_state);
    update_caps_lock_led(caps_lock_state);
  }

  void event_tap_pointing_device_event_callback(CGEventType type, CGEventRef _Nullable event) {
    boost::optional<event_type> pseudo_event_type;
    boost::optional<event_queue::queued_event::event> pseudo_event;

    switch (type) {
      case kCGEventLeftMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button1);
        break;

      case kCGEventLeftMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button1);
        break;

      case kCGEventRightMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button2);
        break;

      case kCGEventRightMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button2);
        break;

      case kCGEventOtherMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button3);
        break;

      case kCGEventOtherMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button3);
        break;

      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventOtherMouseDragged:
        pseudo_event_type = event_type::single;
        pseudo_event = event_queue::queued_event::event(pointing_motion());
        break;

      case kCGEventScrollWheel:
        pseudo_event_type = event_type::single;
        // Set non-zero value for `manipulator::details::base::unset_alone_if_needed`.
        {
          pointing_motion pointing_motion;
          pointing_motion.set_vertical_wheel(1);
          pseudo_event = event_queue::queued_event::event(pointing_motion);
        }
        break;

      case kCGEventNull:
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        break;
    }

    if (pseudo_event_type && pseudo_event) {
      auto e = event_queue::queued_event::event::make_pointing_device_event_from_event_tap_event();
      event_queue::queued_event queued_event(device_id(0),
                                             event_queue::queued_event::event_time_stamp(mach_absolute_time()),
                                             e,
                                             *pseudo_event_type,
                                             *pseudo_event);

      merged_input_event_queue_->push_back_event(queued_event);

      krbn_notification_center::get_instance().input_event_arrived();
    }
  }

  void update_caps_lock_led(bool caps_lock_state) {
    if (core_configuration_) {
      for (const auto& it : hid_manager_.get_hids()) {
        auto& di = (it.second)->get_connected_device().get_identifiers();
        bool manipulate_caps_lock_led = core_configuration_->get_selected_profile().get_device_manipulate_caps_lock_led(di);
        if ((it.second)->is_grabbed() &&
            manipulate_caps_lock_led) {
          (it.second)->set_caps_lock_led_state(caps_lock_state ? led_state::on : led_state::off);
        }
      }
    }
  }

  bool is_pointing_device_grabbed(void) const {
    for (const auto& it : hid_manager_.get_hids()) {
      if ((it.second)->is_pointing_device() &&
          (it.second)->is_grabbed()) {
        return true;
      }
    }
    return false;
  }

  void update_virtual_hid_keyboard(void) {
    if (virtual_hid_device_client_.is_connected()) {
      if (mode_ == mode::grabbing) {
        pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
        properties.country_code = profile_.get_virtual_hid_keyboard().get_country_code();

        virtual_hid_device_client_.initialize_virtual_hid_keyboard(properties);
        return;
      }

      virtual_hid_device_client_.terminate_virtual_hid_keyboard();
    }
  }

  void update_virtual_hid_pointing(void) {
    if (virtual_hid_device_client_.is_connected()) {
      if (mode_ == mode::grabbing) {
        if (is_pointing_device_grabbed() ||
            manipulator_managers_connector_.needs_virtual_hid_pointing()) {
          virtual_hid_device_client_.initialize_virtual_hid_pointing();
          return;
        }
      }

      virtual_hid_device_client_.terminate_virtual_hid_pointing();
    }
  }

  bool is_ignored_device(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_ignore(device.get_connected_device().get_identifiers());
    }

    return false;
  }

  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_disable_built_in_keyboard_if_exists(device.get_connected_device().get_identifiers());
    }
    return false;
  }

  bool need_to_disable_built_in_keyboard(void) const {
    for (const auto& it : hid_manager_.get_hids()) {
      if (get_disable_built_in_keyboard_if_exists(*(it.second))) {
        return true;
      }
    }
    return false;
  }

  void enable_devices(void) {
    for (const auto& it : hid_manager_.get_hids()) {
      if ((it.second)->is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        (it.second)->disable();
      } else {
        (it.second)->enable();
      }
    }
  }

  void output_devices_json(void) const {
    connected_devices connected_devices;
    for (const auto& it : hid_manager_.get_hids()) {
      connected_devices.push_back_device(it.second->get_connected_device());
    }

    auto file_path = constants::get_devices_json_file_path();
    connected_devices.save_to_file(file_path);
  }

  void output_device_details_json(void) const {
    // ----------------------------------------
    std::vector<device_detail> device_details;
    for (const auto& pair : hid_manager_.get_hids()) {
      device_details.push_back(pair.second->make_device_detail());
    }

    std::sort(std::begin(device_details),
              std::end(device_details),
              [](auto& a, auto& b) {
                return a.compare(b);
              });

    auto file_path = constants::get_device_details_json_file_path();
    json_utility::save_to_file(nlohmann::json(device_details), file_path, 0755, 0644);
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
    simple_modifications_manipulator_manager_.invalidate_manipulators();

    for (const auto& device : profile_.get_devices()) {
      for (const auto& pair : device.get_simple_modifications().get_pairs()) {
        if (auto m = make_simple_modifications_manipulator(pair)) {
          auto c = make_device_if_condition(device);
          m->push_back_condition(c);
          simple_modifications_manipulator_manager_.push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_simple_modifications().get_pairs()) {
      if (auto m = make_simple_modifications_manipulator(pair)) {
        simple_modifications_manipulator_manager_.push_back_manipulator(m);
      }
    }
  }

  std::shared_ptr<manipulator::details::conditions::base> make_device_if_condition(const core_configuration::profile::device& device) const {
    nlohmann::json json;
    json["type"] = "device_if";
    json["identifiers"] = nlohmann::json::array();
    json["identifiers"].push_back(nlohmann::json::object());
    json["identifiers"].back()["vendor_id"] = static_cast<int>(device.get_identifiers().get_vendor_id());
    json["identifiers"].back()["product_id"] = static_cast<int>(device.get_identifiers().get_product_id());
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
    complex_modifications_manipulator_manager_.invalidate_manipulators();

    for (const auto& rule : profile_.get_complex_modifications().get_rules()) {
      for (const auto& manipulator : rule.get_manipulators()) {
        auto m = krbn::manipulator::manipulator_factory::make_manipulator(manipulator.get_json(), manipulator.get_parameters());
        for (const auto& c : manipulator.get_conditions()) {
          m->push_back_condition(krbn::manipulator::manipulator_factory::make_condition(c.get_json()));
        }
        complex_modifications_manipulator_manager_.push_back_manipulator(m);
      }
    }
  }

  void update_fn_function_keys_manipulators(void) {
    fn_function_keys_manipulator_manager_.invalidate_manipulators();

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
        fn_function_keys_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
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
          fn_function_keys_manipulator_manager_.push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_fn_function_keys().get_pairs()) {
      if (auto m = make_fn_function_keys_manipulator(pair,
                                                     from_mandatory_modifiers,
                                                     from_optional_modifiers,
                                                     to_modifiers)) {
        fn_function_keys_manipulator_manager_.push_back_manipulator(m);
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
        fn_function_keys_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
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

  virtual_hid_device_client virtual_hid_device_client_;
  boost::signals2::connection client_connected_connection;
  boost::signals2::connection client_disconnected_connection;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;

  std::unique_ptr<event_tap_manager> event_tap_manager_;
  hid_manager hid_manager_;

  std::unordered_map<registry_entry_id, std::deque<std::shared_ptr<grabbable_state_entry>>> grabbable_states_;
  std::unordered_map<registry_entry_id, uint64_t> first_grabbed_event_time_stamps_;

  core_configuration::profile profile_;
  system_preferences system_preferences_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;
  boost::signals2::connection input_event_arrived_connection_;
  boost::signals2::connection manipulator_timer_invoked_connection_;

  std::mutex manipulate_mutex_;
  boost::optional<manipulator::manipulator_timer::timer_id> manipulator_timer_id_;

  std::shared_ptr<event_queue> merged_input_event_queue_;

  manipulator::manipulator_manager simple_modifications_manipulator_manager_;
  std::shared_ptr<event_queue> simple_modifications_applied_event_queue_;

  manipulator::manipulator_manager complex_modifications_manipulator_manager_;
  std::shared_ptr<event_queue> complex_modifications_applied_event_queue_;

  manipulator::manipulator_manager fn_function_keys_manipulator_manager_;
  std::shared_ptr<event_queue> fn_function_keys_applied_event_queue_;

  std::shared_ptr<manipulator::details::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator_;
  manipulator::manipulator_manager post_event_to_virtual_devices_manipulator_manager_;
  std::shared_ptr<event_queue> posted_event_queue_;

  std::unique_ptr<gcd_utility::main_queue_timer> led_monitor_timer_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
};
} // namespace krbn
