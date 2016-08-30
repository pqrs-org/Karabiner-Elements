#pragma once

#include "apple_hid_usage_tables.hpp"
#include "console_user_client.hpp"
#include "constants.hpp"
#include "hid_report.hpp"
#include "human_interface_device.hpp"
#include "iokit_user_client.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "userspace_types.hpp"
#include "virtual_hid_manager_user_client_method.hpp"
#include <thread>

class device_grabber final {
public:
  device_grabber(void) : iokit_user_client_(logger::get_logger(), "org_pqrs_driver_VirtualHIDManager", kIOHIDServerConnectType),
                         console_user_client_() {
    logger::get_logger().info(__PRETTY_FUNCTION__);

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
    logger::get_logger().info(__PRETTY_FUNCTION__);

    if (manager_) {
      IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      CFRelease(manager_);
      manager_ = nullptr;
    }
  }

  void clear_simple_modifications(void) {
    std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

    simple_modifications_.clear();
  }

  void add_simple_modification(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

    simple_modifications_[from_key_code] = to_key_code;
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

    hids_[device] = std::make_unique<human_interface_device>(logger::get_logger(), device);
    auto& dev = hids_[device];

    logger::get_logger().info("matching device: "
                              "manufacturer:{1}, "
                              "product:{2}, "
                              "vendor_id:0x{3:x}, "
                              "product_id:0x{4:x}, "
                              "location_id:0x{5:x}, "
                              "serial_number:{6} "
                              "@ {0}",
                              __PRETTY_FUNCTION__,
                              dev->get_manufacturer(),
                              dev->get_product(),
                              dev->get_vendor_id(),
                              dev->get_product_id(),
                              dev->get_location_id(),
                              dev->get_serial_number_string());

    if (dev->get_serial_number_string() == "org.pqrs.driver.VirtualHIDKeyboard") {
      return;
    }

    if (dev->get_manufacturer() != "pqrs.org") {
      dev->grab(std::bind(&device_grabber::value_callback,
                          this,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3,
                          std::placeholders::_4,
                          std::placeholders::_5,
                          std::placeholders::_6));
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

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        logger::get_logger().info("removal device: "
                                  "vendor_id:0x{1:x}, "
                                  "product_id:0x{2:x}, "
                                  "location_id:0x{3:x} "
                                  "@ {0}",
                                  __PRETTY_FUNCTION__,
                                  dev->get_vendor_id(),
                                  dev->get_product_id(),
                                  dev->get_location_id());

        hids_.erase(it);
      }
    }
  }

  void value_callback(human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {

#if 0
    std::cout << "element" << std::endl
              << "  usage_page:0x" << std::hex << usage_page << std::endl
              << "  usage:0x" << std::hex << usage << std::endl
              << "  type:" << IOHIDElementGetType(element) << std::endl
              << "  length:" << IOHIDValueGetLength(value) << std::endl
              << "  integer_value:" << integer_value << std::endl;
#endif

    switch (usage_page) {
    case kHIDPage_KeyboardOrKeypad:
      if (kHIDUsage_KeyboardErrorUndefined < usage && usage < kHIDUsage_Keyboard_Reserved) {
        bool pressed = integer_value;
        handle_keyboard_event(device, krbn::key_code(usage), pressed);
      }
      break;

    case kHIDPage_AppleVendorTopCase:
      if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
        bool pressed = integer_value;
        handle_keyboard_event(device, krbn::key_code::vk_fn_modifier, pressed);
      }
      break;

    default:
      break;
    }
  }

  bool handle_modifier_flag_event(krbn::key_code key_code, bool pressed) {
    auto operation = pressed ? manipulator::modifier_flag_manager::operation::increase : manipulator::modifier_flag_manager::operation::decrease;

    auto modifier_flag = krbn::types::get_modifier_flag(key_code);
    if (modifier_flag != krbn::modifier_flag::zero) {
      modifier_flag_manager_.manipulate(modifier_flag, operation);

      // reset modifier_flags state if all keys are released.
      if (get_all_devices_pressed_keys_count() == 0) {
        modifier_flag_manager_.reset();
      }

      if (modifier_flag == krbn::modifier_flag::fn) {
        console_user_client_.post_modifier_flags(modifier_flag_manager_.get_io_option_bits());
      } else {
        send_keyboard_input_report();
      }
      return true;
    }

    return false;
  }

  bool handle_function_key_event(krbn::key_code key_code, bool pressed) {
    auto event_type = pressed ? krbn::event_type::key_down : krbn::event_type::key_up;

    if (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12) {
      auto i = static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_f1);
      console_user_client_.post_key(krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_f1) + i),
                                    event_type,
                                    modifier_flag_manager_.get_io_option_bits());
      return true;
    }
    if (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12) {
      auto i = static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_fn_f1);
      console_user_client_.post_key(krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_fn_f1) + i),
                                    event_type,
                                    modifier_flag_manager_.get_io_option_bits());
      return true;
    }

    return false;
  }

  void handle_keyboard_event(human_interface_device& device, krbn::key_code key_code, bool pressed) {
    // ----------------------------------------
    // modify usage
    if (!pressed) {
      auto it = device.get_simple_changed_keys().find(key_code);
      if (it != device.get_simple_changed_keys().end()) {
        key_code = it->second;
        device.get_simple_changed_keys().erase(it);
      }
    } else {
      std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

      auto it = simple_modifications_.find(key_code);
      if (it != simple_modifications_.end()) {
        (device.get_simple_changed_keys())[key_code] = it->second;
        key_code = it->second;
      }
    }

    // modify fn+arrow, function keys
    if (!pressed) {
      auto it = device.get_fn_changed_keys().find(key_code);
      if (it != device.get_fn_changed_keys().end()) {
        key_code = it->second;
        device.get_fn_changed_keys().erase(it);
      }
    } else {
      auto k = static_cast<uint32_t>(key_code);
      auto new_key_code = key_code;
      if (modifier_flag_manager_.pressed(krbn::modifier_flag::fn)) {
        if (k == kHIDUsage_KeyboardReturnOrEnter) {
          new_key_code = krbn::key_code(kHIDUsage_KeypadEnter);
        } else if (k == kHIDUsage_KeyboardDeleteOrBackspace) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardDeleteForward);
        } else if (k == kHIDUsage_KeyboardRightArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardEnd);
        } else if (k == kHIDUsage_KeyboardLeftArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardHome);
        } else if (k == kHIDUsage_KeyboardDownArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardPageDown);
        } else if (k == kHIDUsage_KeyboardUpArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardPageUp);
        } else if (kHIDUsage_KeyboardF1 <= k && k <= kHIDUsage_KeyboardF12) {
          new_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_fn_f1) + k - kHIDUsage_KeyboardF1);
        }
      } else {
        if (kHIDUsage_KeyboardF1 <= k && k <= kHIDUsage_KeyboardF12) {
          new_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_f1) + k - kHIDUsage_KeyboardF1);
        }
      }
      if (key_code != new_key_code) {
        (device.get_fn_changed_keys())[key_code] = new_key_code;
        key_code = new_key_code;
      }
    }

    // ----------------------------------------
    // send input events to virtual devices.
    if (handle_modifier_flag_event(key_code, pressed)) {
      console_user_client_.stop_key_repeat();
      return;
    }
    if (handle_function_key_event(key_code, pressed)) {
      return;
    }

    if (static_cast<uint32_t>(key_code) < kHIDUsage_Keyboard_Reserved) {
      auto usage = static_cast<uint32_t>(key_code);
      if (pressed) {
        pressed_key_usages_.push_back(usage);
        console_user_client_.stop_key_repeat();
      } else {
        pressed_key_usages_.remove(usage);
      }

      send_keyboard_input_report();
    }
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

  size_t get_all_devices_pressed_keys_count(void) const {
    size_t total = 0;
    for (const auto& it : hids_) {
      if (it.second) {
        total += (it.second)->get_pressed_keys_count();
      }
    }
    return total;
  }

  iokit_user_client iokit_user_client_;
  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
  std::list<uint32_t> pressed_key_usages_;

  manipulator::modifier_flag_manager modifier_flag_manager_;
  hid_report::keyboard_input last_keyboard_input_report_;

  std::unordered_map<krbn::key_code, krbn::key_code> simple_modifications_;
  std::mutex simple_modifications_mutex_;

  console_user_client console_user_client_;
};
