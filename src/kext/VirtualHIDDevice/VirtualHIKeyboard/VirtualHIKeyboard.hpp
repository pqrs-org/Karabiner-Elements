#pragma once

#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/hidsystem/IOHIKeyboard.h>
END_IOKIT_INCLUDE;

class org_pqrs_driver_VirtualHIKeyboard final : public IOHIKeyboard {
  OSDeclareDefaultStructors(org_pqrs_driver_VirtualHIKeyboard);
};
