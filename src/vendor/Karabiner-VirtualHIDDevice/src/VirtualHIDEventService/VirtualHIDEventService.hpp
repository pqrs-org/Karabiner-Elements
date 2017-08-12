#pragma once

#include "DiagnosticMacros.hpp"
#include "VersionSignature.hpp"

BEGIN_IOKIT_INCLUDE;
#include "10.11/include/IOKit/hidevent/IOHIDEventService.h"
END_IOKIT_INCLUDE;

#include "karabiner_virtual_hid_device.hpp"

class VIRTUAL_HID_EVENT_SERVICE_CLASS final : public IOHIDEventService {
  OSDeclareDefaultStructors(VIRTUAL_HID_EVENT_SERVICE_CLASS);

public:
  virtual bool handleStart(IOService* provider) override;
  virtual void handleStop(IOService* provider) override;
  virtual OSArray* getReportElements(void) override { return reportElements_; }

  void dispatchKeyboardEvent(pqrs::karabiner_virtual_hid_device::usage_page usagePage,
                             pqrs::karabiner_virtual_hid_device::usage usage,
                             UInt32 value);
  void dispatchKeyUpAllPressedKeys(void);
  void clearKeyboardModifierFlags(void);

private:
  void dispatchKeyUpIfNeeded(pqrs::karabiner_virtual_hid_device::usage_page usagePage,
                             pqrs::karabiner_virtual_hid_device::usage usage);

  OSArray* reportElements_;
  UInt64 pressedKeys_[254];
};
