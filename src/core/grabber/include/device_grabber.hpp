#pragma once

#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <thread>
#include <time.h>

class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(manipulator::event_manipulator& event_manipulator) : event_manipulator_(event_manipulator),
                                                                      queue_(dispatch_queue_create(nullptr, nullptr)),
                                                                      grab_timer_(0),
                                                                      grabbed_(false) {
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
  }

  ~device_grabber(void) {
    cancel_grab_timer();

    if (manager_) {
      IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      CFRelease(manager_);
      manager_ = nullptr;
    }

    dispatch_release(queue_);
  }

  void grab_devices(void) {
    auto __block last_warning_message_time = ::time(nullptr) - 1;

    cancel_grab_timer();

    // ----------------------------------------
    // setup grab_timer_

    std::lock_guard<std::mutex> guard(grab_timer_mutex_);

    grab_timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue_);
    dispatch_source_set_timer(grab_timer_, dispatch_time(DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC), 0.1 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(grab_timer_, ^{
      if (grabbed_) {
        return;
      }

      std::string warning_message;

      if (!event_manipulator_.is_ready()) {
        warning_message = "event_manipulator_ is not ready. Please wait for a while.";
      }

      if (auto product_name = get_key_pressed_device_product_name()) {
        warning_message = std::string("There are pressed down keys in ") + *product_name + ". Please release them.";
      }

      if (!warning_message.empty()) {
        auto time = ::time(nullptr);
        if (last_warning_message_time != time) {
          last_warning_message_time = time;
          logger::get_logger().warn(warning_message);
        }
        return;
      }

      // ----------------------------------------
      // grab devices

      grabbed_ = true;

      {
        std::lock_guard<std::mutex> hids_guard(hids_mutex_);

        for (auto&& it : hids_) {
          unobserve(*(it.second));
          grab(*(it.second));
        }
      }

      event_manipulator_.reset();
      event_manipulator_.grab_mouse_events();

      logger::get_logger().info("devices are grabbed");

      cancel_grab_timer();
    });
    dispatch_resume(grab_timer_);
  }

  void ungrab_devices(void) {
    if (!grabbed_) {
      return;
    }

    grabbed_ = false;

    cancel_grab_timer();

    {
      std::lock_guard<std::mutex> guard(hids_mutex_);

      for (auto&& it : hids_) {
        ungrab(*(it.second));
        observe(*(it.second));
      }
    }

    event_manipulator_.ungrab_mouse_events();
    event_manipulator_.reset();

    logger::get_logger().info("devices are ungrabbed");
  }

  void set_caps_lock_led_state(krbn::led_state state) {
    std::lock_guard<std::mutex> guard(hids_mutex_);

    for (const auto& it : hids_) {
      (it.second)->set_caps_lock_led_state(state);
    }
  }

private:
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

    // ----------------------------------------

    if (grabbed_) {
      grab(*dev);
    } else {
      observe(*dev);
    }

    {
      std::lock_guard<std::mutex> guard(hids_mutex_);

      hids_[device] = std::move(dev);
    }

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

    {
      std::lock_guard<std::mutex> guard(hids_mutex_);

      auto it = hids_.find(device);
      if (it != hids_.end()) {
        auto& dev = it->second;
        if (dev) {
          hids_.erase(it);
        }
      }
    }

    if (is_pointing_device_connected()) {
      event_manipulator_.create_virtual_hid_manager_client();
    } else {
      event_manipulator_.release_virtual_hid_manager_client();
    }

    event_manipulator_.stop_key_repeat();
  }

  void observe(human_interface_device& hid) {
    auto manufacturer = hid.get_manufacturer();
    if (manufacturer && *manufacturer == "pqrs.org") {
      return;
    }

    human_interface_device::value_callback callback;
    hid.observe(callback);
  }

  void unobserve(human_interface_device& hid) {
    auto manufacturer = hid.get_manufacturer();
    if (manufacturer && *manufacturer == "pqrs.org") {
      return;
    }

    hid.unobserve();
  }

  void grab(human_interface_device& hid) {
    auto manufacturer = hid.get_manufacturer();
    if (manufacturer && *manufacturer == "pqrs.org") {
      return;
    }

    // seize device
    hid.grab(std::bind(&device_grabber::value_callback,
                       this,
                       std::placeholders::_1,
                       std::placeholders::_2,
                       std::placeholders::_3,
                       std::placeholders::_4,
                       std::placeholders::_5,
                       std::placeholders::_6));

    // set keyboard led
    event_manipulator_.refresh_caps_lock_led();
  }

  void ungrab(human_interface_device& hid) {
    auto manufacturer = hid.get_manufacturer();
    if (manufacturer && *manufacturer == "pqrs.org") {
      return;
    }

    hid.ungrab();
  }

  void value_callback(human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {
    if (!grabbed_) {
      return;
    }

    auto device_registry_entry_id = manipulator::device_registry_entry_id(device.get_registry_entry_id());

    switch (usage_page) {
    case kHIDPage_KeyboardOrKeypad:
      if (kHIDUsage_KeyboardErrorUndefined < usage && usage < kHIDUsage_Keyboard_Reserved) {
        bool pressed = integer_value;
        event_manipulator_.handle_keyboard_event(device_registry_entry_id, krbn::key_code(usage), pressed);
      }
      break;

    case kHIDPage_AppleVendorTopCase:
      if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
        bool pressed = integer_value;
        event_manipulator_.handle_keyboard_event(device_registry_entry_id, krbn::key_code::vk_fn_modifier, pressed);
      }
      break;

    case kHIDPage_Button:
      event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                               krbn::pointing_event::button,
                                               krbn::pointing_button(usage),
                                               integer_value);
      break;

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

    // reset modifier_flags state if all keys are released.
    if (get_all_devices_pressed_keys_count() == 0) {
      event_manipulator_.reset_modifier_flag_state();
      event_manipulator_.reset_pointing_button_state();
    }
  }

  boost::optional<std::string> get_key_pressed_device_product_name(void) {
    std::lock_guard<std::mutex> guard(hids_mutex_);

    for (const auto& it : hids_) {
      if ((it.second)->get_pressed_keys_count() > 0) {
        if (auto product = (it.second)->get_product()) {
          return *product;
        } else {
          return std::string("(no product name)");
        }
      }
    }

    return boost::none;
  }

  size_t get_all_devices_pressed_keys_count(void) {
    std::lock_guard<std::mutex> guard(hids_mutex_);

    size_t total = 0;
    for (const auto& it : hids_) {
      total += (it.second)->get_pressed_keys_count();
    }
    return total;
  }

  bool is_keyboard_connected(void) {
    std::lock_guard<std::mutex> guard(hids_mutex_);

    for (const auto& it : hids_) {
      if ((it.second)->is_keyboard()) {
        return true;
      }
    }
    return false;
  }

  bool is_pointing_device_connected(void) {
    std::lock_guard<std::mutex> guard(hids_mutex_);

    for (const auto& it : hids_) {
      if ((it.second)->is_pointing_device()) {
        return true;
      }
    }
    return false;
  }

  void cancel_grab_timer(void) {
    std::lock_guard<std::mutex> guard(grab_timer_mutex_);

    if (grab_timer_) {
      dispatch_source_cancel(grab_timer_);
      dispatch_release(grab_timer_);
      grab_timer_ = 0;
    }
  }

  manipulator::event_manipulator& event_manipulator_;
  IOHIDManagerRef _Nullable manager_;

  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
  std::mutex hids_mutex_;

  dispatch_queue_t _Nonnull queue_;
  dispatch_source_t _Nullable grab_timer_;
  std::mutex grab_timer_mutex_;

  std::atomic<bool> grabbed_;
};
