#pragma once

#include "base.hpp"

namespace krbn {
namespace manipulator {
namespace conditions {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    return true;
  }
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
