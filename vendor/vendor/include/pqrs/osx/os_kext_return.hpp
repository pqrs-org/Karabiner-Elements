#pragma once

// pqrs::osx::os_kext_return v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <libkern/OSKextLib.h>
#include <string>

namespace pqrs {
namespace osx {
class os_kext_return final {
public:
  os_kext_return(OSReturn r) : return_(r) {
  }

  OSReturn get(void) const {
    return return_;
  }

  std::string to_string(void) const {
#define PQRS_OSX_OS_KEXT_RETURN_TO_STRING(OSRETURN) \
  case OSRETURN:                                    \
    return #OSRETURN;

    switch (return_) {
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSReturnSuccess);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnInternalError);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNoMemory);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNoResources);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNotPrivileged);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnInvalidArgument);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNotFound);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnBadData);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnSerialization);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnUnsupported);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnDisabled);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNotAKext);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnValidation);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnAuthentication);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnDependencies);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnArchNotFound);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnCache);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnDeferred);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnBootLevel);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnNotLoadable);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnLoadedVersionDiffers);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnDependencyLoadError);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnLinkError);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnStartStopError);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnInUse);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnTimeout);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnStopping);
      PQRS_OSX_OS_KEXT_RETURN_TO_STRING(kOSKextReturnSystemPolicy);
    }

#undef PQRS_OSX_OS_KEXT_RETURN_TO_STRING

    return std::string("Unknown OSReturn (") + std::to_string(return_) + ")";
  }

  bool success(void) const {
    return return_ == kOSReturnSuccess;
  }

  bool system_policy(void) const {
    return return_ == kOSKextReturnSystemPolicy;
  }

  operator bool(void) const {
    return success();
  }

private:
  OSReturn return_;
};

inline std::ostream& operator<<(std::ostream& stream, const os_kext_return& value) {
  return stream << value.to_string();
}
} // namespace osx
} // namespace pqrs
