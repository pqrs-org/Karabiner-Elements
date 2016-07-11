#pragma once

#include <memory>
#include <string>
#include <unordered_map>

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

    // kHIDUsage_GD_Mouse
    {
      auto device_matching_dictionary = create_device_matching_dictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
      if (device_matching_dictionary) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }

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
              << "  " << dev->get_manufacturer() << std::endl
              << "  " << dev->get_product() << std::endl;

    if (dev->get_serial_number_string() == "org.pqrs.driver.VirtualHIDKeyboard") {
      dev->open();
      self->virtual_keyboard_ = dev;
    }

    //if (dev->get_manufacturer() != "pqrs.org") {
    if (dev->get_manufacturer() == "Apple Inc.") {
      dev->grab(input_report_callback, self);
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

  static void input_report_callback(
      void* _Nullable context,
      IOReturn result,
      void* _Nullable sender,
      IOHIDReportType type,
      uint32_t reportID,
      uint8_t* _Nonnull report,
      CFIndex reportLength) {
    if (!context) {
      return;
    }

    std::cout << "input_report_callback" << std::endl;
    std::cout << "  type:" << type << std::endl;
    std::cout << "  reportID:" << reportID << std::endl;
    for (CFIndex i = 0; i < reportLength; ++i) {
      std::cout << "  report[" << i << "]:0x" << std::hex << static_cast<int>(report[i]) << std::endl;
    }

    if (result == kIOReturnSuccess) {
      auto self = static_cast<event_grabber*>(context);

      if (auto vk = self->virtual_keyboard_.lock()) {
        IOReturn r = vk->set_report(type, reportID, report, reportLength);
        std::cout << "IOReturn " << r << std::endl;
      }
    }

#if 0
    if (value) {
      auto element = IOHIDValueGetElement(value);
      auto integerValue = IOHIDValueGetIntegerValue(value);

      if (element) {
        auto usagePage = IOHIDElementGetUsagePage(element);
        auto usage = IOHIDElementGetUsage(element);

        std::cout << "type: " << IOHIDElementGetType(element) << std::endl;

        switch (usagePage) {
        case kHIDPage_KeyboardOrKeypad:
          if (usage == kHIDUsage_KeyboardErrorRollOver ||
              usage == kHIDUsage_KeyboardPOSTFail ||
              usage == kHIDUsage_KeyboardErrorUndefined ||
              usage >= kHIDUsage_GD_Reserved) {
            // do nothing
          } else {
            // bool keyDown = (integerValue == 1);
            std::cout << "inputValueCallback usagePage:" << usagePage << " usage:" << usage << " value:" << integerValue << std::endl;
            if (usage == kHIDUsage_KeyboardEscape) {
              exit(0);
            }
          }
          break;

        default:
          std::cout << "inputValueCallback unknown usagePage:" << usagePage << " usage:" << usage << std::endl;
        }
      }
    }
#endif
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::shared_ptr<human_interface_device>> hids_;
  std::weak_ptr<human_interface_device> virtual_keyboard_;
};
