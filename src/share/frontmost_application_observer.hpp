#pragma once

#include "frontmost_application_observer_objc.h"
#include "logger.hpp"

namespace krbn {
class frontmost_application_observer final {
public:
  frontmost_application_observer(krbn_frontmost_application_observer_callback callback) : observer_(nullptr) {
    krbn_frontmost_application_observer_initialize(&observer_, callback);
  }

  ~frontmost_application_observer(void) {
    krbn_frontmost_application_observer_terminate(&observer_);
  }

private:
  krbn_frontmost_application_observer_objc* observer_;
};
} // namespace krbn
