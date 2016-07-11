#pragma once

class human_interface_device final {
public:
  human_interface_device(IOHIDDeviceRef _Nonnull device) : device_(device),
                                                           report_buffer_(nullptr),
                                                           report_buffer_size_(0),
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

  void grab(IOHIDReportCallback _Nonnull report_callback, void* _Nullable report_callback_context) {
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

    report_buffer_size_ = get_max_input_report_size();
    if (report_buffer_size_ > 0) {
      report_buffer_ = new uint8_t[report_buffer_size_];
      IOHIDDeviceRegisterInputReportCallback(device_, report_buffer_, report_buffer_size_, report_callback, report_callback_context);
    }

    IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    grabbed_ = true;
  }

  void ungrab(void) {
    if (!device_) {
      return;
    }

    if (!grabbed_) {
      return;
    }

    IOHIDDeviceUnscheduleFromRunLoop(device_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputReportCallback(device_, nullptr, 0, nullptr, nullptr);

    IOReturn r = close();
    if (r != kIOReturnSuccess) {
      std::cerr << "Failed to IOHIDDeviceClose: " << std::hex << r << std::endl;
      return;
    }

    if (report_buffer_) {
      delete[] report_buffer_;
      report_buffer_size_ = 0;
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
  uint8_t* _Nullable report_buffer_;
  CFIndex report_buffer_size_;
  IOHIDQueueRef _Nullable queue_;

  bool grabbed_;
};
