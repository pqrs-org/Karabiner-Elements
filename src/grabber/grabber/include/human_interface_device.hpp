#pragma once

class human_interface_device final {
public:
  human_interface_device(IOHIDDeviceRef _Nonnull device) : device_(device),
                                                           queue_(nullptr),
                                                           transaction_(nullptr),
                                                           grabbed_(false) {
    CFRetain(device_);

    auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone);
    if (elements) {
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        // Skip unnecessary usages.
        auto element = static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i)));
        auto usage_page = IOHIDElementGetUsagePage(element);
        auto usage = IOHIDElementGetUsage(element);

        if (usage_page == kHIDPage_KeyboardOrKeypad) {
          if (usage < kHIDUsage_KeyboardA ||
              usage >= kHIDUsage_Keyboard_Reserved) {
            continue;
          }
        }

        // Add to elements_
        auto key = elements_key(usage_page, usage);
        if (elements_.find(key) == elements_.end()) {
          CFRetain(element);
          elements_[key] = element;
        }
      }
      CFRelease(elements);
    }
  }

  ~human_interface_device(void) {
    unschedule();

    if (grabbed_) {
      ungrab();
    } else {
      close();
    }

    if (transaction_) {
      CFRelease(transaction_);
    }

    if (queue_) {
      CFRelease(queue_);
    }

    for (const auto& it : elements_) {
      CFRelease(it.second);
    }
    elements_.clear();

    CFRelease(device_);
  }

  IOReturn open(IOOptionBits options = kIOHIDOptionsTypeNone) {
    return IOHIDDeviceOpen(device_, options);
  }

  IOReturn close(void) {
    return IOHIDDeviceClose(device_, kIOHIDOptionsTypeNone);
  }

  void create_transaction(void) {
    if (transaction_) {
      return;
    }

    transaction_ = IOHIDTransactionCreate(kCFAllocatorDefault, device_, kIOHIDTransactionDirectionTypeOutput, kIOHIDOptionsTypeNone);
  }

  void schedule(void) {
    IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    if (queue_) {
      IOHIDQueueScheduleWithRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
    if (transaction_) {
      IOHIDTransactionScheduleWithRunLoop(transaction_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
  }

  void unschedule(void) {
    if (transaction_) {
      IOHIDTransactionUnscheduleFromRunLoop(transaction_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
    if (queue_) {
      IOHIDQueueUnscheduleFromRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
    IOHIDQueueUnscheduleFromRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  }

  void grab(IOHIDCallback _Nonnull value_available_callback, void* _Nullable callback_context) {
    if (!device_) {
      return;
    }

    if (grabbed_) {
      ungrab();
    }

    IOReturn r = open(kIOHIDOptionsTypeSeizeDevice);
    if (r != kIOReturnSuccess) {
      std::cerr << "Failed to IOHIDDeviceOpen: " << std::hex << r << std::endl;
      return;
    }

    const CFIndex depth = 1024;
    queue_ = IOHIDQueueCreate(kCFAllocatorDefault, device_, depth, kIOHIDOptionsTypeNone);
    if (queue_) {
      // Add all elements
      for (const auto& it : elements_) {
        IOHIDQueueAddElement(queue_, it.second);
      }

      IOHIDQueueRegisterValueAvailableCallback(queue_, value_available_callback, callback_context);

      IOHIDQueueStart(queue_);
    }

    schedule();

    grabbed_ = true;
  }

  void ungrab(void) {
    if (!device_) {
      return;
    }

    if (!grabbed_) {
      return;
    }

    unschedule();

    if (queue_) {
      IOHIDQueueRegisterValueAvailableCallback(queue_, nullptr, nullptr);
      IOHIDQueueStop(queue_);

      // Remove all elements
      for (const auto& it : elements_) {
        IOHIDQueueRemoveElement(queue_, it.second);
      }

      CFRelease(queue_);
      queue_ = nullptr;
    }

    IOReturn r = close();
    if (r != kIOReturnSuccess) {
      std::cerr << "Failed to IOHIDDeviceClose: " << std::hex << r << std::endl;
      return;
    }

    grabbed_ = false;
  }

  IOReturn set_value(uint32_t usage_page, uint32_t usage, IOHIDValueRef _Nonnull value) {
    auto value_element = IOHIDValueGetElement(value);
    if (!value_element) {
      return kIOReturnError;
    }

    IOHIDElementRef element = nullptr;
    auto value_device = IOHIDElementGetDevice(value_element);
    if (value_device == device_) {
      element = value_element;
      CFRetain(value);
    } else {
      auto key = elements_key(usage_page, usage);
      auto it = elements_.find(key);
      if (it == elements_.end()) {
        value = nullptr;
      } else {
        element = it->second;

        // create value for device_.
        auto p = IOHIDValueGetBytePtr(value);
        if (p) {
          value = IOHIDValueCreateWithBytesNoCopy(kCFAllocatorDefault, element, mach_absolute_time(), p, IOHIDValueGetLength(value));
        } else {
          value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, mach_absolute_time(), IOHIDValueGetIntegerValue(value));
        }
      }
    }

    if (!element) {
      return kIOReturnError;
    }

    if (!transaction_) {
      return kIOReturnError;
    }

    std::cout << std::hex << "0x" << usage_page << " 0x" << usage << std::endl;

    IOHIDTransactionAddElement(transaction_, element);
    IOHIDTransactionSetValue(transaction_, element, value, kIOHIDOptionsTypeNone);
    auto result = IOHIDTransactionCommit(transaction_);

#if 0
    auto result = IOHIDDeviceSetValue(device_, element, value);
#endif

    if (value) {
      CFRelease(value);
    }
    return result;
  }

  IOReturn set_report(IOHIDReportType report_type, CFIndex report_id, const uint8_t* _Nonnull report, CFIndex report_length) {
    return IOHIDDeviceSetReport(device_, report_type, report_id, report, report_length);
  }

  long get_max_input_report_size(void) const {
    long value = 0;
    get_long_property(CFSTR(kIOHIDMaxInputReportSizeKey), value);
    return value;
  }

  long get_vendor_id(void) const {
    long value = 0;
    get_long_property(CFSTR(kIOHIDVendorIDKey), value);
    return value;
  }

  long get_product_id(void) const {
    long value = 0;
    get_long_property(CFSTR(kIOHIDProductIDKey), value);
    return value;
  }

  long get_location_id(void) const {
    long value = 0;
    get_long_property(CFSTR(kIOHIDLocationIDKey), value);
    return value;
  }

  std::string get_manufacturer(void) const {
    std::string value;
    get_string_property(CFSTR(kIOHIDManufacturerKey), value);
    return value;
  }

  std::string get_product(void) const {
    std::string value;
    get_string_property(CFSTR(kIOHIDProductKey), value);
    return value;
  }

  std::string get_serial_number_string(void) const {
    std::string value;
    get_string_property(CFSTR(kIOHIDSerialNumberKey), value);
    return value;
  }

  std::string get_transport(void) const {
    std::string value;
    get_string_property(CFSTR(kIOHIDTransportKey), value);
    return value;
  }

  bool get_long_property(const CFStringRef _Nonnull key, long& value) const {
    if (!device_) {
      return false;
    }

    auto property = IOHIDDeviceGetProperty(device_, key);
    if (!property) {
      return false;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(property)) {
      return false;
    }

    return CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberLongType, &value);
  }

  bool get_string_property(const CFStringRef _Nonnull key, std::string& value) const {
    if (!device_) {
      return false;
    }

    auto property = IOHIDDeviceGetProperty(device_, key);
    if (!property) {
      return false;
    }

    if (CFStringGetTypeID() != CFGetTypeID(property)) {
      return false;
    }

    auto p = CFStringGetCStringPtr(static_cast<CFStringRef>(property), kCFStringEncodingUTF8);
    if (!p) {
      value.clear();
    } else {
      value = p;
    }
    return true;
  }

private:
  uint64_t elements_key(uint32_t usage_page, uint32_t usage) {
    return (static_cast<uint64_t>(usage_page) << 32 | usage);
  }

  IOHIDDeviceRef _Nonnull device_;
  IOHIDQueueRef _Nullable queue_;
  IOHIDTransactionRef _Nullable transaction_;
  std::unordered_map<uint64_t, IOHIDElementRef> elements_;

  bool grabbed_;
};
