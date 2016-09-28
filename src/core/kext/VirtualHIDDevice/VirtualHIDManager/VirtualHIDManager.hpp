#pragma once

#include "DiagnosticMacros.hpp"
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

  org_pqrs_driver_VirtualHIDPointing* getVirtualHIDPointing(void) {
    if (!virtualHIDPointingDetector_) {
      return nullptr;
    }
    return OSDynamicCast(org_pqrs_driver_VirtualHIDPointing, virtualHIDPointingDetector_->getService());
  }

private:
#include "include/ServiceDetector.hpp"

  ServiceDetector* virtualHIDPointingDetector_;
};
