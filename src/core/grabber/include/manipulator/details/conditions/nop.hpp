#pragma once

#include "manipulator/details/conditions/base.hpp"

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual bool is_fulfilled(const manipulator_environment& manipulator_environment) const {
    return true;
  }
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
