#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "event_tap_manager.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "manipulator/details/collapse_lazy_events.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "physical_keyboard_repeat_detector.hpp"
#include "pressed_physical_keys_counter.hpp"
#include "spdlog_utility.hpp"
#include "system_preferences.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(virtual_hid_device_client& virtual_hid_device_client,
                 manipulator::event_manipulator& event_manipulator) : virtual_hid_device_client_(virtual_hid_device_client),
                                                                      event_manipulator_(event_manipulator),
                                                                      profile_(nlohmann::json()),
                                                                      mode_(mode::observing),
                                                                      is_grabbable_callback_log_reducer_(logger::get_logger()),
                                                                      suspended_(false) {
    virtual_hid_device_client_disconnected_connection = virtual_hid_device_client_.client_disconnected.connect(
        boost::bind(&device_grabber::virtual_hid_device_client_disconnected_callback, this));

    {
      auto manipulator = std::make_unique<krbn::manipulator::details::collapse_lazy_events>();
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      collapse_lazy_events_manipulator_manager_.push_back_manipulator(std::move(ptr));
    }

    manipulator_managers_connector_.emplace_back_connection(simple_modifications_manipulator_manager_,
                                                            merged_input_event_queue_,
                                                            simple_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(fn_function_keys_manipulator_manager_,
                                                            simple_modifications_applied_event_queue_,
                                                            fn_function_keys_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(collapse_lazy_events_manipulator_manager_,
                                                            fn_function_keys_applied_event_queue_,
                                                            lazy_collapsed_event_queue_);

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

    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        // std::make_pair(hid_usage_page::consumer, hid_usage::csmr_consumercontrol),
        // std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

  ~device_grabber(void) {
    // Release manager_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      stop_grabbing();

      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }

      led_monitor_timer_ = nullptr;

      virtual_hid_device_client_disconnected_connection.disconnect();
    });
  }

  void start_grabbing(const std::string& user_core_configuration_file_path) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      mode_ = mode::grabbing;

      event_manipulator_.reset();

      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_manager here.
      event_tap_manager_ = std::make_unique<event_tap_manager>(std::bind(&device_grabber::caps_lock_state_changed_callback, this, std::placeholders::_1));

      configuration_monitor_ = std::make_unique<configuration_monitor>(logger::get_logger(),
                                                                       user_core_configuration_file_path,
                                                                       [this](std::shared_ptr<core_configuration> core_configuration) {
                                                                         core_configuration_ = core_configuration;

                                                                         is_grabbable_callback_log_reducer_.reset();
                                                                         set_profile(core_configuration_->get_selected_profile());
                                                                         event_manipulator_.set_profile(core_configuration_->get_selected_profile());
                                                                         grab_devices();
                                                                       });
    });
  }

  void stop_grabbing(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      configuration_monitor_ = nullptr;

      ungrab_devices();

      mode_ = mode::observing;

      event_manipulator_.reset();

      event_tap_manager_ = nullptr;
    });
  }

  void grab_devices(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (const auto& it : hids_) {
        if ((it.second)->is_grabbable() == human_interface_device::grabbable_state::ungrabbable_permanently) {
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
      for (auto&& it : hids_) {
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

  void set_profile(const core_configuration::profile& profile) {
    profile_ = profile;

    update_simple_modifications_manipulators();
    update_fn_function_keys_manipulators();
  }

  void unset_profile(void) {
    profile_ = core_configuration::profile(nlohmann::json());

    manipulator_managers_connector_.invalidate_manipulators();
  }

  void set_system_preferences_values(const system_preferences::values& values) {
    system_preferences_values_ = values;

    update_fn_function_keys_manipulators();
  }

private:
  enum class mode {
    observing,
    grabbing,
  };

  void virtual_hid_device_client_disconnected_callback(void) {
    stop_grabbing();
  }

  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    iokit_utility::log_matching_device(logger::get_logger(), device);

    auto dev = std::make_unique<human_interface_device>(logger::get_logger(), device);
    dev->set_is_grabbable_callback(std::bind(&device_grabber::is_grabbable_callback, this, std::placeholders::_1));
    dev->set_grabbed_callback(std::bind(&device_grabber::grabbed_callback, this, std::placeholders::_1));
    dev->set_ungrabbed_callback(std::bind(&device_grabber::ungrabbed_callback, this, std::placeholders::_1));
    dev->set_disabled_callback(std::bind(&device_grabber::disabled_callback, this, std::placeholders::_1));
    dev->set_value_callback(std::bind(&device_grabber::value_callback,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
    dev->observe();

    hids_[device] = std::move(dev);

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.initialize_virtual_hid_pointing();
    } else {
      event_manipulator_.terminate_virtual_hid_pointing();
    }

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      grab_devices();
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    iokit_utility::log_removal_device(logger::get_logger(), device);

    boost::optional<device_id> device_id;

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        device_id = dev->get_device_id();
        hids_.erase(it);
      }
    }

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.initialize_virtual_hid_pointing();
    } else {
      event_manipulator_.terminate_virtual_hid_pointing();
    }

    if (device_id) {
      event_manipulator_.erase_all_active_modifier_flags(*device_id, true);
      event_manipulator_.erase_all_active_pointing_buttons(*device_id, true);

      physical_keyboard_repeat_detector_.erase(*device_id);
      pressed_physical_keys_counter_.erase_all_matched_events(*device_id);
    }

    event_manipulator_.stop_key_repeat();

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      grab_devices();
    }
  }

  void value_callback(human_interface_device& device,
                      event_queue& event_queue) {
    bool pressed_physical_keys_counter_updated = false;

    // Update physical_keyboard_repeat_detector_, pressed_physical_keys_counter_
    {
      for (const auto& queued_event : event_queue.get_events()) {
        if (queued_event.get_valid()) {
          if (auto key_code = queued_event.get_event().get_key_code()) {
            physical_keyboard_repeat_detector_.set(queued_event.get_device_id(), *key_code, queued_event.get_event_type());
          }
          if (pressed_physical_keys_counter_.update(queued_event)) {
            pressed_physical_keys_counter_updated = true;
          }
        }
      }
    }

    if (device.is_grabbed() && !device.get_disabled()) {
      for (const auto& queued_event : event_queue.get_events()) {
        merged_input_event_queue_.push_back_event(queued_event);
      }

      manipulator_managers_connector_.manipulate(mach_absolute_time());

      for (const auto& queued_event : lazy_collapsed_event_queue_.get_events()) {
        // logger::get_logger().info("event_time: {0} current_time:{1}", queued_event.get_time_stamp(), mach_absolute_time());

        if (queued_event.get_valid()) {
          auto device_id = queued_event.get_device_id();
          auto time_stamp = queued_event.get_time_stamp();

          if (auto key_code = queued_event.get_event().get_key_code()) {
            event_manipulator_.handle_keyboard_event(device_id,
                                                     time_stamp,
                                                     *key_code,
                                                     queued_event.get_event_type() == event_type::key_down);

          } else if (auto pointing_button = queued_event.get_event().get_pointing_button()) {
            event_manipulator_.handle_pointing_event(device_id,
                                                     time_stamp,
                                                     pointing_event::button,
                                                     *pointing_button,
                                                     queued_event.get_event_type() == event_type::key_down);

          } else {
            if (auto integer_value = queued_event.get_event().get_integer_value()) {
              if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_x) {
                event_manipulator_.handle_pointing_event(device_id,
                                                         time_stamp,
                                                         pointing_event::x,
                                                         boost::none,
                                                         *integer_value);

              } else if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_y) {
                event_manipulator_.handle_pointing_event(device_id,
                                                         time_stamp,
                                                         pointing_event::y,
                                                         boost::none,
                                                         *integer_value);

              } else if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_vertical_wheel) {
                event_manipulator_.handle_pointing_event(device_id,
                                                         time_stamp,
                                                         pointing_event::vertical_wheel,
                                                         boost::none,
                                                         *integer_value);

              } else if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_horizontal_wheel) {
                event_manipulator_.handle_pointing_event(device_id,
                                                         time_stamp,
                                                         pointing_event::horizontal_wheel,
                                                         boost::none,
                                                         *integer_value);
              }
            }
          }
        }
      }

      lazy_collapsed_event_queue_.clear_events();

      // reset modifier_flags state if all keys are released.
      if (pressed_physical_keys_counter_updated &&
          pressed_physical_keys_counter_.empty(device.get_device_id())) {
        event_manipulator_.erase_all_active_modifier_flags(device.get_device_id(), false);
        event_manipulator_.erase_all_active_pointing_buttons(device.get_device_id(), false);
      }
    }
  }

  human_interface_device::grabbable_state is_grabbable_callback(human_interface_device& device) {
    if (is_ignored_device(device)) {
      // If we need to disable the built-in keyboard, we have to grab it.
      if (device.is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        // Do nothing
      } else {
        logger::get_logger().info("{0} is ignored.", device.get_name_for_log());
        return human_interface_device::grabbable_state::ungrabbable_permanently;
      }
    }

    // ----------------------------------------
    // Ungrabbable while event_manipulator_ is not ready.

    auto ready_state = event_manipulator_.is_ready();
    if (ready_state != manipulator::event_manipulator::ready_state::ready) {
      std::string message = "event_manipulator_ is not ready. ";
      switch (ready_state) {
        case manipulator::event_manipulator::ready_state::ready:
          break;
        case manipulator::event_manipulator::ready_state::virtual_hid_device_client_is_not_ready:
          message += "(virtual_hid_device_client is not ready) ";
          break;
        case manipulator::event_manipulator::ready_state::virtual_hid_keyboard_is_not_ready:
          message += "(virtual_hid_keyboard is not ready) ";
          break;
      }
      message += "Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Ungrabbable while key repeating

    if (physical_keyboard_repeat_detector_.is_repeating(device.get_device_id())) {
      is_grabbable_callback_log_reducer_.warn(std::string("We cannot grab ") + device.get_name_for_log() + " while a key is repeating.");
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Ungrabbable while pointing button is pressed.

    if (pressed_physical_keys_counter_.is_pointing_button_pressed(device.get_device_id())) {
      // We should not grab the device while a button is pressed since we cannot release the button.
      // (To release the button, we have to send a hid report to the device. But we cannot do it.)

      is_grabbable_callback_log_reducer_.warn(std::string("We cannot grab ") + device.get_name_for_log() + " while mouse buttons are pressed.");
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------

    return human_interface_device::grabbable_state::grabbable;
  }

  void grabbed_callback(human_interface_device& device) {
    // set keyboard led
    if (event_tap_manager_) {
      bool state = false;
      if (auto s = event_tap_manager_->get_caps_lock_state()) {
        state = *s;
      }
      update_caps_lock_led(state);
    }
  }

  void ungrabbed_callback(human_interface_device& device) {
    // stop key repeat
    event_manipulator_.stop_key_repeat();

    manipulator_managers_connector_.run_device_ungrabbed_callback(device.get_device_id(),
                                                                  mach_absolute_time());
  }

  void disabled_callback(human_interface_device& device) {
    // stop key repeat
    event_manipulator_.stop_key_repeat();
  }

  void caps_lock_state_changed_callback(bool caps_lock_state) {
    event_manipulator_.set_caps_lock_state(caps_lock_state);
    update_caps_lock_led(caps_lock_state);
  }

  void update_caps_lock_led(bool caps_lock_state) {
    // Update LED.
    for (const auto& it : hids_) {
      if ((it.second)->is_grabbed()) {
        (it.second)->set_caps_lock_led_state(caps_lock_state ? led_state::on : led_state::off);
      }
    }
  }

  bool is_keyboard_connected(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_keyboard()) {
        return true;
      }
    }
    return false;
  }

  bool is_pointing_device_connected(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_pointing_device()) {
        return true;
      }
    }
    return false;
  }

  boost::optional<const core_configuration::profile::device&> find_device_configuration(const human_interface_device& device) {
    if (core_configuration_) {
      for (const auto& d : core_configuration_->get_selected_profile().get_devices()) {
        if (d.get_identifiers() == device.get_connected_device().get_identifiers()) {
          return d;
        }
      }
    }
    return boost::none;
  }

  bool is_ignored_device(const human_interface_device& device) {
    if (auto s = find_device_configuration(device)) {
      return s->get_ignore();
    }

    if (device.is_pointing_device()) {
      return true;
    }

    if (auto v = device.get_vendor_id()) {
      if (auto p = device.get_product_id()) {
        // Touch Bar on MacBook Pro 2016
        if (*v == vendor_id(0x05ac) && *p == product_id(0x8600)) {
          return true;
        }
      }
    }

    return false;
  }

  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) {
    if (auto s = find_device_configuration(device)) {
      return s->get_disable_built_in_keyboard_if_exists();
    }
    return false;
  }

  bool need_to_disable_built_in_keyboard(void) {
    for (const auto& it : hids_) {
      if (get_disable_built_in_keyboard_if_exists(*(it.second))) {
        return true;
      }
    }
    return false;
  }

  void enable_devices(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        (it.second)->disable();
      } else {
        (it.second)->enable();
      }
    }
  }

  void output_devices_json(void) {
    connected_devices connected_devices;
    for (const auto& it : hids_) {
      if ((it.second)->is_pqrs_device()) {
        continue;
      }
      connected_devices.push_back_device(it.second->get_connected_device());
    }

    auto file_path = constants::get_devices_json_file_path();
    if (connected_devices.save_to_file(file_path)) {
      chmod(file_path, 0644);
    }
  }

  void update_simple_modifications_manipulators(void) {
    simple_modifications_manipulator_manager_.invalidate_manipulators();

    for (const auto& pair : profile_.get_simple_modifications_key_code_map(logger::get_logger())) {
      auto manipulator = std::make_unique<manipulator::details::basic>(manipulator::details::event_definition(
                                                                           pair.first,
                                                                           std::unordered_set<manipulator::details::event_definition::modifier>({
                                                                               manipulator::details::event_definition::modifier::any,
                                                                           })),
                                                                       manipulator::details::event_definition(pair.second));
      std::unique_ptr<manipulator::details::base> ptr = std::move(manipulator);
      simple_modifications_manipulator_manager_.push_back_manipulator(std::move(ptr));
    }
  }

  void update_fn_function_keys_manipulators(void) {
    fn_function_keys_manipulator_manager_.invalidate_manipulators();

    std::unordered_set<manipulator::details::event_definition::modifier> from_modifiers;
    std::unordered_set<manipulator::details::event_definition::modifier> to_modifiers;

    if (system_preferences_values_.get_keyboard_fn_state()) {
      // f1 -> f1
      // fn+f1 -> display_brightness_decrement

      from_modifiers.insert(manipulator::details::event_definition::modifier::fn);
      from_modifiers.insert(manipulator::details::event_definition::modifier::any);
      to_modifiers.insert(manipulator::details::event_definition::modifier::fn);

    } else {
      // f1 -> display_brightness_decrement
      // fn+f1 -> f1

      from_modifiers.insert(manipulator::details::event_definition::modifier::any);

      // fn+f1 ... fn+f12 -> f1 .. f12

      for (const auto& key_code : std::vector<key_code>({
               key_code::f1,
               key_code::f2,
               key_code::f3,
               key_code::f4,
               key_code::f5,
               key_code::f6,
               key_code::f7,
               key_code::f8,
               key_code::f9,
               key_code::f10,
               key_code::f11,
               key_code::f12,
           })) {
        auto manipulator = std::make_unique<manipulator::details::basic>(manipulator::details::event_definition(
                                                                             key_code,
                                                                             std::unordered_set<manipulator::details::event_definition::modifier>({
                                                                                 manipulator::details::event_definition::modifier::fn,
                                                                                 manipulator::details::event_definition::modifier::any,
                                                                             })),
                                                                         manipulator::details::event_definition(
                                                                             key_code,
                                                                             std::unordered_set<manipulator::details::event_definition::modifier>({
                                                                                 manipulator::details::event_definition::modifier::fn,
                                                                             })));
        std::unique_ptr<manipulator::details::base> ptr = std::move(manipulator);
        fn_function_keys_manipulator_manager_.push_back_manipulator(std::move(ptr));
      }
    }

    // from_modifiers+f1 -> display_brightness_decrement ...

    for (const auto& pair : profile_.get_fn_function_keys_key_code_map(logger::get_logger())) {
      auto manipulator = std::make_unique<manipulator::details::basic>(manipulator::details::event_definition(
                                                                           pair.first,
                                                                           from_modifiers),
                                                                       manipulator::details::event_definition(
                                                                           pair.second,
                                                                           to_modifiers));
      std::unique_ptr<manipulator::details::base> ptr = std::move(manipulator);
      fn_function_keys_manipulator_manager_.push_back_manipulator(std::move(ptr));
    }

    // fn+return_or_enter -> keypad_enter ...

    auto pairs = std::vector<std::pair<key_code, key_code>>({
        std::make_pair(key_code::return_or_enter, key_code::keypad_enter),
        std::make_pair(key_code::delete_or_backspace, key_code::delete_forward),
        std::make_pair(key_code::right_arrow, key_code::end),
        std::make_pair(key_code::left_arrow, key_code::home),
        std::make_pair(key_code::down_arrow, key_code::page_down),
        std::make_pair(key_code::up_arrow, key_code::page_up),
    });
    for (const auto& p : pairs) {
      auto manipulator = std::make_unique<manipulator::details::basic>(manipulator::details::event_definition(
                                                                           p.first,
                                                                           std::unordered_set<manipulator::details::event_definition::modifier>({
                                                                               manipulator::details::event_definition::modifier::fn,
                                                                               manipulator::details::event_definition::modifier::any,
                                                                           })),
                                                                       manipulator::details::event_definition(
                                                                           p.second,
                                                                           std::unordered_set<manipulator::details::event_definition::modifier>({
                                                                               manipulator::details::event_definition::modifier::fn,
                                                                           })));
      std::unique_ptr<manipulator::details::base> ptr = std::move(manipulator);
      fn_function_keys_manipulator_manager_.push_back_manipulator(std::move(ptr));
    }
  }

  virtual_hid_device_client& virtual_hid_device_client_;
  manipulator::event_manipulator& event_manipulator_;

  boost::signals2::connection virtual_hid_device_client_disconnected_connection;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;

  std::unique_ptr<event_tap_manager> event_tap_manager_;
  IOHIDManagerRef _Nullable manager_;
  physical_keyboard_repeat_detector physical_keyboard_repeat_detector_;
  pressed_physical_keys_counter pressed_physical_keys_counter_;

  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;

  core_configuration::profile profile_;
  system_preferences::values system_preferences_values_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;

  event_queue merged_input_event_queue_;

  manipulator::manipulator_manager simple_modifications_manipulator_manager_;
  event_queue simple_modifications_applied_event_queue_;

  manipulator::manipulator_manager fn_function_keys_manipulator_manager_;
  event_queue fn_function_keys_applied_event_queue_;

  manipulator::manipulator_manager collapse_lazy_events_manipulator_manager_;
  event_queue lazy_collapsed_event_queue_;

  std::unique_ptr<gcd_utility::main_queue_timer> led_monitor_timer_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
};
} // namespace krbn
