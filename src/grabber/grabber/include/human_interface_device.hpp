#pragma once

class human_interface_device final {
public:
  human_interface_device(IOHIDDeviceRef _Nonnull device) : device_(device),
                                                           queue_(nullptr),
                                                           grabbed_(false) {
    CFRetain(device_);

    auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone);
    if (elements) {
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        auto element = static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i)));
        uint32_t usage_page = IOHIDElementGetUsagePage(element);
        uint32_t usage = IOHIDElementGetUsage(element);
        uint64_t key = (static_cast<uint64_t>(usage_page) << 32 | usage);

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

  void schedule(void) {
    IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    if (queue_) {
      IOHIDQueueScheduleWithRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
  }

  void unschedule(void) {
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
      uint64_t key = (static_cast<uint64_t>(usage_page) << 32) | usage;
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

    std::cout << "set_value usage_page:" << IOHIDElementGetUsagePage(element)
              << " usage:" << IOHIDElementGetUsage(element)
              << std::endl;

    std::cout << "  device:" << device_ << std::endl
              << "  element-device:" << IOHIDElementGetDevice(element) << std::endl;

    auto result = IOHIDDeviceSetValue(device_, element, value);
    if (value) {
      CFRelease(value);
    }
    return result;
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
  IOHIDDeviceRef _Nonnull device_;
  IOHIDQueueRef _Nullable queue_;
  std::unordered_map<uint64_t, IOHIDElementRef> elements_;

  bool grabbed_;
};
