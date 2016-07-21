#pragma once

#include "hid_report.hpp"
#include "human_interface_device.hpp"

class event_grabber final {
public:
  event_grabber(void) {
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
      dev->open();
      dev->create_transaction();
      dev->schedule();
      std::cout << "set virtual_keyboard_ " << std::endl;
      self->virtual_keyboard_ = dev;
    } else {
      //if (dev->get_manufacturer() != "pqrs.org") {
      if (dev->get_manufacturer() == "Apple Inc.") {
        dev->grab(queue_value_available_callback, self);
      }
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
    if (it == (self->hids_).end()) {
      std::cout << "unknown device has been removed" << std::endl;
    } else {
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

        if (usage == kHIDUsage_KeyboardEscape) {
          exit(0);
        }

        if (usage == kHIDUsage_KeyboardCapsLock) {
          if (self) {
            auto vk = (self->virtual_keyboard_).lock();
            if (vk) {
              hid_report::keyboard_input report;
              if (IOHIDValueGetIntegerValue(value)) {
                report.keys[0] = kHIDUsage_KeyboardSpacebar;
              }
              vk->set_report(kIOHIDReportTypeInput, 0, reinterpret_cast<const uint8_t*>(&report), sizeof(report));
            }
          }
        }

        if (kHIDUsage_KeyboardA <= usage) {
          if (self) {
            auto vk = (self->virtual_keyboard_).lock();
            if (vk) {
              //std::cout << vk->set_value(usage_page, usage, value) << std::endl;
            }
          }
        }
      }

      CFRelease(value);
    }
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::shared_ptr<human_interface_device>> hids_;
  std::weak_ptr<human_interface_device> virtual_keyboard_;
};
