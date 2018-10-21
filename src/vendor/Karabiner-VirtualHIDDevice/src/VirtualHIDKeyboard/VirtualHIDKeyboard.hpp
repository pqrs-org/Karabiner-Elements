#pragma once

#include "DiagnosticMacros.hpp"
#include "VersionSignature.hpp"
#include "karabiner_virtual_hid_device.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/hid/IOHIDDevice.h>
END_IOKIT_INCLUDE;

class VIRTUAL_HID_KEYBOARD_CLASS final : public IOHIDDevice {
  OSDeclareDefaultStructors(VIRTUAL_HID_KEYBOARD_CLASS);

public:
  virtual bool handleStart(IOService* provider) override;

  // ----------------------------------------

  virtual OSString* newManufacturerString() const override {
    return OSString::withCString("pqrs.org");
  }

  virtual OSString* newProductString() const override {
    return OSString::withCString("Karabiner VirtualHIDKeyboard");
  }

  virtual OSString* newSerialNumberString() const override {
    return OSString::withCString(VIRTUAL_HID_KEYBOARD_SERIAL_NUMBER_STRING);
  }

  // ----------------------------------------

  virtual OSNumber* newVendorIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0x16c0), 32);
  }

  virtual OSNumber* newProductIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0x27db), 32);
  }

  virtual OSNumber* newLocationIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
  }

  virtual OSNumber* newCountryCodeNumber() const override;

  // ----------------------------------------

  virtual OSNumber* newPrimaryUsagePageNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
  }

  virtual OSNumber* newPrimaryUsageNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Keyboard), 32);
  }

  // ----------------------------------------

  virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;

  // ----------------------------------------

  virtual IOReturn setReport(IOMemoryDescriptor* report,
                             IOHIDReportType reportType,
                             IOOptionBits options) override {
    IOReturn result = kIOReturnError;

    uint8_t report_id = 0;
    if (report->readBytes(0, &report_id, sizeof(report_id)) == 0) {
      return kIOReturnBadArgument;
    }

    // The report_id is described at `reportDescriptor_` in `VirtualHIDKeyboard.cpp`.

    switch (report_id) {
    case 5: {
      // LED

      struct __attribute__((packed)) led_report {
        uint8_t report_id;
        uint8_t state;
      } led_report;

      led_report.report_id = 6;
      // report[0] is report_id. (The LED output report id == 5)
      // The led state is stored in `report[1]`.
      report->readBytes(1, &(led_report.state), sizeof(led_report.state));

      // Post LED report

      if (auto buffer = IOBufferMemoryDescriptor::withBytes(&led_report, sizeof(led_report), kIODirectionNone)) {
        result = handleReport(buffer, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);

        buffer->release();
      }

      break;
    }
    }

    return result;
  }

  // ----------------------------------------

  static void setCountryCode(uint8_t value);
};
