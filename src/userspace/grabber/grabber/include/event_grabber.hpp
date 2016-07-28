#pragma once

#include "apple_hid_usage_tables.hpp"
#include "hid_report.hpp"
#include "human_interface_device.hpp"
#include "user_client.hpp"
#include "virtual_hid_manager_user_client_method.hpp"

class event_grabber final {
public:
  event_grabber(void) {
    if (!user_client_.open("org_pqrs_driver_VirtualHIDManager", kIOHIDServerConnectType)) {
      std::cerr << "Failed to open user_client." << std::endl;
      return;
    }

    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = create_device_matching_dictionaries();
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, device_removal_callback, this);

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
  CFDictionaryRef _Nonnull create_device_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    auto device_matching_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!device_matching_dictionary) {
      goto finish;
    }

    // usage_page
    if (!usage_page) {
      goto finish;
    } else {
      auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
      if (!number) {
        goto finish;
      }
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDDeviceUsagePageKey), number);
      CFRelease(number);
    }

    // usage (The usage is only valid if the usage page is also defined)
    if (!usage) {
      goto finish;
    } else {
      auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
      if (!number) {
        goto finish;
      }
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDDeviceUsageKey), number);
      CFRelease(number);
    }

  finish:
    return device_matching_dictionary;
  }

  CFArrayRef _Nullable create_device_matching_dictionaries(void) {
    auto device_matching_dictionaries = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    if (!device_matching_dictionaries) {
      return nullptr;
    }

    // kHIDUsage_GD_Keyboard
    {
      auto device_matching_dictionary = create_device_matching_dictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
      if (device_matching_dictionary) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }

#if 0
    // kHIDUsage_Csmr_ConsumerControl
    {
      auto device_matching_dictionary = create_device_matching_dictionary(kHIDPage_Consumer, kHIDUsage_Csmr_ConsumerControl);
      if (device_matching_dictionary) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }

    // kHIDUsage_GD_Mouse
    {
      auto device_matching_dictionary = create_device_matching_dictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
      if (device_matching_dictionary) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }
#endif

    return device_matching_dictionaries;
  }

  static void device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<event_grabber*>(context);
    if (!self) {
      return;
    }

    if (!device) {
      return;
    }

    auto dev = std::make_shared<human_interface_device>(device);
    if (!dev) {
      return;
    }

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

    //if (dev->get_manufacturer() != "pqrs.org") {
    if (dev->get_manufacturer() == "Apple Inc.") {
      dev->grab(queue_value_available_callback, self);
    }

    (self->hids_)[device] = dev;
  }

  static void device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<event_grabber*>(context);
    if (!self) {
      return;
    }

    if (!device) {
      return;
    }

    auto it = (self->hids_).find(device);
    if (it != (self->hids_).end()) {
      auto dev = it->second;
      if (dev) {
        std::cout << "removal vendor_id:0x" << std::hex << dev->get_vendor_id() << " product_id:0x" << std::hex << dev->get_product_id() << std::endl;
        (self->hids_).erase(it);
      }
    }
  }

  static void queue_value_available_callback(void* _Nullable context, IOReturn result, void* _Nullable sender) {
    auto self = static_cast<event_grabber*>(context);
    auto queue = static_cast<IOHIDQueueRef>(sender);

    while (true) {
      auto value = IOHIDQueueCopyNextValueWithTimeout(queue, 0.);
      if (!value) {
        break;
      }

      auto element = IOHIDValueGetElement(value);
      if (element) {
        auto usage_page = IOHIDElementGetUsagePage(element);
        auto usage = IOHIDElementGetUsage(element);

        std::cout << "element" << std::endl
                  << "  usage_page:0x" << std::hex << usage_page << std::endl
                  << "  usage:0x" << std::hex << usage << std::endl
                  << "  type:" << IOHIDElementGetType(element) << std::endl
                  << "  length:" << IOHIDValueGetLength(value) << std::endl
                  << "  integer_value:" << IOHIDValueGetIntegerValue(value) << std::endl;

        switch (usage_page) {
        case kHIDPage_KeyboardOrKeypad: {
          bool pressed = IOHIDValueGetIntegerValue(value);
          if (self) {
            self->handle_keyboard_event(usage, pressed);
          }
          break;
        }

        case kHIDPage_AppleVendorTopCase:
          if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
            if (self) {
              hid_report::apple_keyboard_input report;
              report.key_code = 0x3f;
              report.key_down = IOHIDValueGetIntegerValue(value);
              kern_return_t kr = IOConnectCallStructMethod(self->user_client_.get_connect(),
                                                           static_cast<uint32_t>(virtual_hid_manager_user_client_method::apple_keyboard_input_report),
                                                           static_cast<const void*>(&report), sizeof(report),
                                                           nullptr, 0);
              if (kr != KERN_SUCCESS) {
                std::cerr << "failed to sent report: 0x" << std::hex << kr << std::dec << std::endl;
              }
            }
          }
          break;

        default:
          break;
        }
      }

      CFRelease(value);
    }
  }

  void handle_keyboard_event(uint32_t usage, bool pressed) {
    // ----------------------------------------
    // modify usage
    if (usage == kHIDUsage_KeyboardCapsLock) {
      usage = kHIDUsage_KeyboardDeleteOrBackspace;
    }

    // ----------------------------------------
    if (pressed) {
      pressing_key_usages_.push_back(usage);
    } else {
      pressing_key_usages_.remove(usage);
    }

    // ----------------------------------------
    hid_report::keyboard_input report;

    while (pressing_key_usages_.size() > sizeof(report.keys)) {
      pressing_key_usages_.pop_front();
    }

    int i = 0;
    for (const auto& u : pressing_key_usages_) {
      report.keys[i] = u;
      ++i;
    }

    kern_return_t kr = IOConnectCallStructMethod(user_client_.get_connect(),
                                                 static_cast<uint32_t>(virtual_hid_manager_user_client_method::keyboard_input_report),
                                                 static_cast<const void*>(&report), sizeof(report),
                                                 nullptr, 0);
    if (kr != KERN_SUCCESS) {
      std::cerr << "failed to sent report: 0x" << std::hex << kr << std::dec << std::endl;
    }
  }

  user_client user_client_;
  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::shared_ptr<human_interface_device>> hids_;
  std::list<uint32_t> pressing_key_usages_;
};
