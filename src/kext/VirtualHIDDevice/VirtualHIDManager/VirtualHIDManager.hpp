#pragma once

#include "DiagnosticMacros.hpp"
#include "VirtualHIDKeyboard.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOService.h>
END_IOKIT_INCLUDE;

class org_pqrs_driver_VirtualHIDManager final : public IOService {
  OSDeclareDefaultStructors(org_pqrs_driver_VirtualHIDManager);

public:
  virtual bool start(IOService* provider) override;
  virtual void stop(IOService* provider) override;

  org_pqrs_driver_VirtualHIDKeyboard* getVirtualHIDKeyboard(void) { return virtualHIDKeyboard_; }

private:
  static bool virtualHIDKeyboardMatchedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier);
  static bool virtualHIDKeyboardTerminatedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier);

  IONotifier* virtualHIDKeyboardMatchedNotifier_;
  IONotifier* virtualHIDKeyboardTerminatedNotifier_;
  org_pqrs_driver_VirtualHIDKeyboard* virtualHIDKeyboard_;
};
