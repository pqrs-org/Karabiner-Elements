#pragma once

#include "DiagnosticMacros.hpp"
#include "VersionSignature.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/hid/IOHIDDevice.h>
END_IOKIT_INCLUDE;

class VIRTUAL_HID_POINTING_CLASS final : public IOHIDDevice {
  OSDeclareDefaultStructors(VIRTUAL_HID_POINTING_CLASS);

public:
  virtual bool handleStart(IOService* provider) override;

  // ----------------------------------------

  virtual OSString* newManufacturerString() const override {
    return OSString::withCString("pqrs.org");
  }

  virtual OSString* newProductString() const override {
    return OSString::withCString("Karabiner VirtualHIDPointing");
  }

  virtual OSString* newSerialNumberString() const override {
    return OSString::withCString(VIRTUAL_HID_POINTING_SERIAL_NUMBER_STRING);
  }

  // ----------------------------------------

  virtual OSNumber* newVendorIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0x16c0), 32);
  }

  virtual OSNumber* newProductIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0x27da), 32);
  }

  virtual OSNumber* newLocationIDNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
  }

  // ----------------------------------------

  virtual OSNumber* newPrimaryUsagePageNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
  }

  virtual OSNumber* newPrimaryUsageNumber() const override {
    return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Mouse), 32);
  }

  // ----------------------------------------

  virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
};
