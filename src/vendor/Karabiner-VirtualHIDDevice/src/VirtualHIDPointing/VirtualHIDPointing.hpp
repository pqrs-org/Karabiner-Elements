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

  virtual OSString* newManufacturerString() const override;
  virtual OSString* newProductString() const override;
  virtual OSNumber* newVendorIDNumber() const override;
  virtual OSNumber* newProductIDNumber() const override;
  virtual OSNumber* newPrimaryUsageNumber() const override;
  virtual OSNumber* newPrimaryUsagePageNumber() const override;
  virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
  virtual OSString* newSerialNumberString() const override;
  virtual OSNumber* newLocationIDNumber() const override;

  static const char* serialNumberCString(void) {
    return VIRTUAL_HID_POINTING_CLASS_STRING;
  }
};
