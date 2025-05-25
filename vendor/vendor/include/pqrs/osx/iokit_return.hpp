#pragma once

// pqrs::osx::iokit_return v1.3

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/IOKitLib.h>
#include <iostream>
#include <string>

namespace pqrs {
namespace osx {
class iokit_return final {
public:
  iokit_return(IOReturn r) : return_(r) {
  }

  IOReturn get(void) const {
    return return_;
  }

  std::string to_string(void) const {
#define PQRS_OSX_IOKIT_RETURN_TO_STRING(IORETURN) \
  case IORETURN:                                  \
    return #IORETURN;

    switch (return_) {
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnSuccess);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoMemory);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoResources);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnIPCError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoDevice);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotPrivileged);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnBadArgument);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnLockedRead);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnLockedWrite);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnExclusiveAccess);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnBadMessageID);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnUnsupported);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnVMError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnInternalError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnIOError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnCannotLock);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotOpen);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotReadable);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotWritable);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotAligned);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnBadMedia);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnStillOpen);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnRLDError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnDMAError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnBusy);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnTimeout);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnOffline);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotReady);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotAttached);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoChannels);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoSpace);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnPortExists);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnCannotWire);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoInterrupt);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoFrames);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnMessageTooLarge);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotPermitted);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoPower);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoMedia);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnUnformattedMedia);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnUnsupportedMode);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnUnderrun);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnOverrun);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnDeviceError);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoCompletion);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnAborted);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNoBandwidth);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotResponding);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnIsoTooOld);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnIsoTooNew);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnNotFound);
      PQRS_OSX_IOKIT_RETURN_TO_STRING(kIOReturnInvalid);
    }

#undef PQRS_OSX_IOKIT_RETURN_TO_STRING

    return std::string("Unknown IOReturn (") + std::to_string(return_) + ")";
  }

  bool success(void) const {
    return return_ == kIOReturnSuccess;
  }

  bool exclusive_access(void) const {
    return return_ == kIOReturnExclusiveAccess;
  }

  bool not_permitted(void) const {
    return return_ == kIOReturnNotPermitted;
  }

  operator bool(void) const {
    return return_ == kIOReturnSuccess;
  }

private:
  IOReturn return_;
};

inline std::ostream& operator<<(std::ostream& stream, const iokit_return& value) {
  return stream << value.to_string();
}
} // namespace osx
} // namespace pqrs
