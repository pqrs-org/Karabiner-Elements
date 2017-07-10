#pragma once

#include "manipulator_environment.hpp"

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class base {
protected:
  base(void) {
  }

public:
  virtual ~base(void) {
  }

  virtual bool is_fulfilled(const manipulator_environment& manipulator_environment) const = 0;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
