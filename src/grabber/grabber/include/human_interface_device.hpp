#pragma once

class human_interface_device final {
public:
  human_interface_device(IOHIDDeviceRef _Nonnull device) : device_(device),
                                                           queue_(nullptr),
                                                           grabbed_(false) {
    CFRetain(device_);
  }

  ~human_interface_device(void) {
    if (grabbed_) {
      ungrab();
    } else {
      close();
    }

    if (queue_) {
      CFRelease(queue_);
    }

    CFRelease(device_);
  }

  IOReturn open(IOOptionBits options = kIOHIDOptionsTypeNone) {
    return IOHIDDeviceOpen(device_, options);
  }

  IOReturn close(void) {
    return IOHIDDeviceClose(device_, kIOHIDOptionsTypeNone);
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
      auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone);
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        IOHIDQueueAddElement(queue_, static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i))));
      }

      IOHIDQueueRegisterValueAvailableCallback(queue_, value_available_callback, callback_context);

      IOHIDQueueStart(queue_);
    }

    IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDQueueScheduleWithRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    grabbed_ = true;
  }

  void ungrab(void) {
    if (!device_) {
      return;
    }

    if (!grabbed_) {
      return;
    }

    IOHIDQueueUnscheduleFromRunLoop(queue_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceUnscheduleFromRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    if (queue_) {
      IOHIDQueueRegisterValueAvailableCallback(queue_, nullptr, nullptr);
      IOHIDQueueStop(queue_);

      // Remove all elements
      auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone);
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        IOHIDQueueRemoveElement(queue_, static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i))));
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

  long get_max_input_report_size(void) {
    long value = 0;
    get_long_property_(CFSTR(kIOHIDMaxInputReportSizeKey), value);
    return value;
  }

  long get_vendor_id(void) {
    long value = 0;
    get_long_property_(CFSTR(kIOHIDVendorIDKey), value);
    return value;
  }

  long get_product_id(void) {
    long value = 0;
    get_long_property_(CFSTR(kIOHIDProductIDKey), value);
    return value;
  }

  long get_location_id(void) {
    long value = 0;
    get_long_property_(CFSTR(kIOHIDLocationIDKey), value);
    return value;
  }

  std::string get_manufacturer(void) {
    std::string value;
    get_string_property_(CFSTR(kIOHIDManufacturerKey), value);
    return value;
  }

  std::string get_product(void) {
    std::string value;
    get_string_property_(CFSTR(kIOHIDProductKey), value);
    return value;
  }

  std::string get_serial_number_string(void) {
    std::string value;
    get_string_property_(CFSTR(kIOHIDSerialNumberKey), value);
    return value;
  }

  IOReturn set_report(IOHIDReportType reportType,
                      CFIndex reportID,
                      const uint8_t* _Nonnull report,
                      CFIndex reportLength) {
    return IOHIDDeviceSetReport(device_, reportType, reportID, report, reportLength);
  }

private:
  bool get_long_property_(const CFStringRef _Nonnull key, long& value) {
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

  bool get_string_property_(const CFStringRef _Nonnull key, std::string& value) {
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

  IOHIDDeviceRef _Nonnull device_;
  IOHIDQueueRef _Nullable queue_;

  bool grabbed_;
};
