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

  static void setCapsLockDelayMilliseconds(pqrs::karabiner_virtual_hid_device::milliseconds value);
  static void setCountryCode(uint8_t value);
};
