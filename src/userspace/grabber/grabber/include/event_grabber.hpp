#pragma once

#include "apple_hid_usage_tables.hpp"
#include "console_user_client.hpp"
#include "constants.hpp"
#include "hid_report.hpp"
#include "human_interface_device.hpp"
#include "iokit_user_client.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "modifier_flag_manager.hpp"
#include "userspace_defs.h"
#include "virtual_hid_manager_user_client_method.hpp"

class event_grabber final {
public:
  event_grabber(void) : iokit_user_client_(logger::get_logger(), "org_pqrs_driver_VirtualHIDManager", kIOHIDServerConnectType),
                        console_user_client_(modifier_flag_manager_) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
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

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
  }

  ~event_grabber(void) {
    if (manager_) {
      IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
      CFRelease(manager_);
      manager_ = nullptr;
    }
  }

private:
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<event_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    hids_[device] = std::make_unique<human_interface_device>(device);
    auto& dev = hids_[device];

    std::cout << "matching: " << std::endl
              << "  vendor_id:0x" << std::hex << dev->get_vendor_id() << std::endl
              << "  product_id:0x" << std::hex << dev->get_product_id() << std::endl
              << "  location_id:0x" << std::hex << dev->get_location_id() << std::endl
              << "  serial_number:" << dev->get_serial_number_string() << std::endl
              << "  manufacturer:" << dev->get_manufacturer() << std::endl
              << "  product:" << dev->get_product() << std::endl
              << "  transport:" << dev->get_transport() << std::endl;

    if (dev->get_serial_number_string() == "org.pqrs.driver.VirtualHIDKeyboard") {
      return;
    }

    if (dev->get_manufacturer() != "pqrs.org") {
      if (dev->get_manufacturer() == "Apple Inc.") {
        dev->grab(boost::bind(&event_grabber::value_callback, this, _1, _2, _3, _4, _5));
      }
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<event_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        std::cout << "removal vendor_id:0x" << std::hex << dev->get_vendor_id() << " product_id:0x" << std::hex << dev->get_product_id() << std::endl;
        hids_.erase(it);
      }
    }
  }

  void value_callback(IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {

    std::cout << "element" << std::endl
              << "  usage_page:0x" << std::hex << usage_page << std::endl
              << "  usage:0x" << std::hex << usage << std::endl
              << "  type:" << IOHIDElementGetType(element) << std::endl
              << "  length:" << IOHIDValueGetLength(value) << std::endl
              << "  integer_value:" << integer_value << std::endl;

    switch (usage_page) {
    case kHIDPage_KeyboardOrKeypad: {
      bool pressed = integer_value;
      handle_keyboard_event(usage_page, usage, pressed);
      break;
    }

    case kHIDPage_AppleVendorTopCase:
      if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
        bool pressed = integer_value;
        handle_keyboard_event(usage_page, usage, pressed);
      }
      break;

    default:
      break;
    }
  }

  bool handle_modifier_flag_event(uint32_t usage_page, uint32_t usage, bool pressed) {
    auto operation = pressed ? modifier_flag_manager::operation::increase : modifier_flag_manager::operation::decrease;

    switch (usage_page) {
    case kHIDPage_KeyboardOrKeypad: {
      auto key = modifier_flag_manager::physical_keys::end_;

      switch (usage) {
      case kHIDUsage_KeyboardLeftControl:
        key = modifier_flag_manager::physical_keys::left_control;
        break;
      case kHIDUsage_KeyboardLeftShift:
        key = modifier_flag_manager::physical_keys::left_shift;
        break;
      case kHIDUsage_KeyboardLeftAlt:
        key = modifier_flag_manager::physical_keys::left_option;
        break;
      case kHIDUsage_KeyboardLeftGUI:
        key = modifier_flag_manager::physical_keys::left_command;
        break;
      case kHIDUsage_KeyboardRightControl:
        key = modifier_flag_manager::physical_keys::right_control;
        break;
      case kHIDUsage_KeyboardRightShift:
        key = modifier_flag_manager::physical_keys::right_shift;
        break;
      case kHIDUsage_KeyboardRightAlt:
        key = modifier_flag_manager::physical_keys::right_option;
        break;
      case kHIDUsage_KeyboardRightGUI:
        key = modifier_flag_manager::physical_keys::right_command;
        break;
      }

      if (key != modifier_flag_manager::physical_keys::end_) {
        modifier_flag_manager_.manipulate(key, operation);
        send_keyboard_input_report();
        return true;
      }
      break;
    }

    case kHIDPage_AppleVendorTopCase:
      if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
        modifier_flag_manager_.manipulate(modifier_flag_manager::physical_keys::fn, operation);
        console_user_client_.post_modifier_flags();
        return true;
      }
      break;
    }

    return false;
  }

  bool handle_function_key_event(uint32_t usage_page, uint32_t usage, bool pressed) {
    if (usage_page != kHIDPage_KeyboardOrKeypad) {
      return false;
    }

    auto event_type = pressed ? KRBN_EVENT_TYPE_KEY_DOWN : KRBN_EVENT_TYPE_KEY_UP;

#define POST_KEY(KEY)                                               \
  case kHIDUsage_Keyboard##KEY:                                     \
    console_user_client_.post_key(KRBN_KEY_CODE_##KEY, event_type); \
    return true;

    switch (usage) {
      POST_KEY(F1);
      POST_KEY(F2);
      POST_KEY(F3);
      POST_KEY(F4);
      POST_KEY(F5);
      POST_KEY(F6);
      POST_KEY(F7);
      POST_KEY(F8);
      POST_KEY(F9);
      POST_KEY(F10);
      POST_KEY(F11);
      POST_KEY(F12);
    }

    return false;
  }

  void handle_keyboard_event(uint32_t usage_page, uint32_t usage, bool pressed) {
    // ----------------------------------------
    // modify usage
    if (usage == kHIDUsage_KeyboardCapsLock) {
      usage = kHIDUsage_KeyboardDeleteOrBackspace;
    }

    // ----------------------------------------
    if (handle_modifier_flag_event(usage_page, usage, pressed)) {
      console_user_client_.stop_key_repeat();
      return;
    }
    if (handle_function_key_event(usage_page, usage, pressed)) {
      return;
    }

    if (pressed) {
      pressed_key_usages_.push_back(usage);
      console_user_client_.stop_key_repeat();
    } else {
      pressed_key_usages_.remove(usage);
    }

    send_keyboard_input_report();
  }

  void send_keyboard_input_report(void) {
    // make report
    hid_report::keyboard_input report;
    report.modifiers = modifier_flag_manager_.get_hid_report_bits();

    while (pressed_key_usages_.size() > sizeof(report.keys)) {
      pressed_key_usages_.pop_front();
    }

    int i = 0;
    for (const auto& u : pressed_key_usages_) {
      report.keys[i] = u;
      ++i;
    }

    // send new report only if it is changed from last report.
    if (last_keyboard_input_report_ != report) {
      last_keyboard_input_report_ = report;

      auto kr = iokit_user_client_.call_struct_method(static_cast<uint32_t>(virtual_hid_manager_user_client_method::keyboard_input_report),
                                                      static_cast<const void*>(&report), sizeof(report),
                                                      nullptr, 0);
      if (kr != KERN_SUCCESS) {
        logger::get_logger().error("failed to sent report: 0x{0:x}", kr);
      }
    }
  }

  iokit_user_client iokit_user_client_;
  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
  std::list<uint32_t> pressed_key_usages_;

  modifier_flag_manager modifier_flag_manager_;
  hid_report::keyboard_input last_keyboard_input_report_;

  console_user_client console_user_client_;
};
