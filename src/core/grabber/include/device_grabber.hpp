#pragma once

#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "iopm_client.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(manipulator::event_manipulator& event_manipulator) : event_manipulator_(event_manipulator),
                                                                      mode_(mode::observing),
                                                                      is_grabbable_callback_log_reducer_(logger::get_logger()),
                                                                      suspended_(false) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard),
        // std::make_pair(kHIDPage_Consumer, kHIDUsage_Csmr_ConsumerControl),
        // std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }

    iopm_client_ = std::make_unique<iopm_client>(logger::get_logger(),
                                                 std::bind(&device_grabber::iopm_client_callback, this, std::placeholders::_1));
  }

  ~device_grabber(void) {
    // Release manager_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      iopm_client_ = nullptr;

      ungrab_devices();

      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }
    });
  }

  void grab_devices(bool change_mode = true) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (change_mode) {
        mode_ = mode::grabbing;
      }

      event_manipulator_.reset();
      event_manipulator_.grab_mouse_events();

      for (const auto& it : hids_) {
        (it.second)->grab();
      }
    });
  }

  void ungrab_devices(bool change_mode = true) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (change_mode) {
        mode_ = mode::observing;
      }

      for (auto&& it : hids_) {
        (it.second)->ungrab();
        (it.second)->observe();
      }

      event_manipulator_.ungrab_mouse_events();
      event_manipulator_.reset();

      logger::get_logger().info("Connected devices are ungrabbed");
    });
  }

  void suspend(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (!suspended_) {
        logger::get_logger().info("device_grabber::suspend");

        suspended_ = true;

        if (mode_ == mode::grabbing) {
          ungrab_devices(false);
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
          grab_devices(false);
        }
      }
    });
  }

  void set_caps_lock_led_state(krbn::led_state state) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (const auto& it : hids_) {
        (it.second)->set_caps_lock_led_state(state);
      }
    });
  }

private:
  enum class mode {
    observing,
    grabbing,
  };

  void iopm_client_callback(uint32_t message_type) {
    switch (message_type) {
    case kIOMessageSystemWillSleep:
      suspend();
      break;

    case kIOMessageSystemWillPowerOn:
      resume();
      break;

    default:
      break;
    }
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
    dev->set_value_callback(std::bind(&device_grabber::value_callback,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4,
                                      std::placeholders::_5,
                                      std::placeholders::_6));

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      dev->grab();
    } else {
      dev->observe();
    }

    hids_[device] = std::move(dev);

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.create_virtual_hid_manager_client();
    } else {
      event_manipulator_.release_virtual_hid_manager_client();
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

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.create_virtual_hid_manager_client();
    } else {
      event_manipulator_.release_virtual_hid_manager_client();
    }

    event_manipulator_.stop_key_repeat();
  }

  void value_callback(human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {
    if (!device.is_grabbed()) {
      return;
    }

    auto device_registry_entry_id = manipulator::device_registry_entry_id(device.get_registry_entry_id());

    if (auto key_code = krbn::types::get_key_code(usage_page, usage)) {
      bool pressed = integer_value;
      event_manipulator_.handle_keyboard_event(device_registry_entry_id, *key_code, pressed);

    } else if (auto pointing_button = krbn::types::get_pointing_button(usage_page, usage)) {
      event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                               krbn::pointing_event::button,
                                               *pointing_button,
                                               integer_value);

    } else {
      switch (usage_page) {
      case kHIDPage_GenericDesktop:
        if (usage == kHIDUsage_GD_X) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   krbn::pointing_event::x,
                                                   boost::none,
                                                   integer_value);
        }
        if (usage == kHIDUsage_GD_Y) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   krbn::pointing_event::y,
                                                   boost::none,
                                                   integer_value);
        }
        if (usage == kHIDUsage_GD_Wheel) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   krbn::pointing_event::vertical_wheel,
                                                   boost::none,
                                                   integer_value);
        }
        break;

      case kHIDPage_Consumer:
        if (usage == kHIDUsage_Csmr_ACPan) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   krbn::pointing_event::horizontal_wheel,
                                                   boost::none,
                                                   integer_value);
        }
        break;

      default:
        break;
      }
    }

    // reset modifier_flags state if all keys are released.
    if (get_all_devices_pressed_keys_count() == 0) {
      event_manipulator_.reset_modifier_flag_state();
      event_manipulator_.reset_pointing_button_state();
    }
  }

  human_interface_device::grabbable_state is_grabbable_callback(human_interface_device& device) {
    if (!event_manipulator_.is_ready()) {
      is_grabbable_callback_log_reducer_.warn("event_manipulator_ is not ready. Please wait for a while.");
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }
    return human_interface_device::grabbable_state::grabbable;
  }

  void grabbed_callback(human_interface_device& device) {
    // set keyboard led
    event_manipulator_.refresh_caps_lock_led();
  }

  size_t get_all_devices_pressed_keys_count(void) {
    size_t total = 0;
    for (const auto& it : hids_) {
      total += (it.second)->get_pressed_keys_count();
    }
    return total;
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

  void output_devices_json(void) {
    nlohmann::json json;
    json = nlohmann::json::array();

    for (const auto& it : hids_) {
      if ((it.second)->is_pqrs_device()) {
        continue;
      }

      nlohmann::json j({
          {"identifiers", {}},
          {"descriptions", {}},
      });
      if (auto vendor_id = (it.second)->get_vendor_id()) {
        j["identifiers"]["vendor_id"] = static_cast<uint32_t>(*vendor_id);
      }
      if (auto product_id = (it.second)->get_product_id()) {
        j["identifiers"]["product_id"] = static_cast<uint32_t>(*product_id);
      }
      j["identifiers"]["is_keyboard"] = (it.second)->is_keyboard();
      j["identifiers"]["is_pointing_device"] = (it.second)->is_pointing_device();
      if (auto manufacturer = (it.second)->get_manufacturer()) {
        j["descriptions"]["manufacturer"] = boost::trim_copy(*manufacturer);
      }
      if (auto product = (it.second)->get_product()) {
        j["descriptions"]["product"] = boost::trim_copy(*product);
      }
      j["ignore"] = is_ignored_device(*(it.second));

      if (!j.empty()) {
        json.push_back(j);
      }
    }

    std::ofstream stream(constants::get_devices_json_file_path());
    if (stream) {
      stream << std::setw(4) << json << std::endl;
      chmod(constants::get_devices_json_file_path(), 0644);
    }
  }

  manipulator::event_manipulator& event_manipulator_;
  IOHIDManagerRef _Nullable manager_;
  std::unique_ptr<iopm_client> iopm_client_;

  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
};
