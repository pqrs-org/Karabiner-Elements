#pragma once

#include "DiagnosticMacros.hpp"
#include "VirtualHIDKeyboard.hpp"
#include "VirtualHIDPointing.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOService.h>
END_IOKIT_INCLUDE;

class org_pqrs_driver_VirtualHIDManager final : public IOService {
  OSDeclareDefaultStructors(org_pqrs_driver_VirtualHIDManager);

public:
  virtual bool init(OSDictionary* dictionary = 0) override;
  virtual void free(void) override;
  virtual bool start(IOService* provider) override;
  virtual void stop(IOService* provider) override;

  void attachClient(void);
  void detachClient(void);

  void terminateVirtualHIDKeyboard(void);
  void terminateVirtualHIDPointing(void);

  IOReturn handleHIDKeyboardReport(IOMemoryDescriptor* report);
  IOReturn handleHIDPointingReport(IOMemoryDescriptor* report);

private:
  // We have to attach virtualHIDKeyboard_ and virtualHIDPointing_ to provider_
  // in order to ensure that org_pqrs_driver_VirtualHIDKeyboard::free will be called.
  IOService* provider_;
  size_t attachedClientCount_;
  org_pqrs_driver_VirtualHIDKeyboard* virtualHIDKeyboard_;
  org_pqrs_driver_VirtualHIDPointing* virtualHIDPointing_;
};
