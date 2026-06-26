#pragma once

#include "base.hpp"

namespace krbn {
namespace manipulator {
namespace conditions {
class nop final : public base {
public:
  nop() : base() {
  }

  ~nop() override {
  }

  bool is_fulfilled(const condition_context& condition_context,
                    const manipulator_environment& manipulator_environment) const override {
    return true;
  }
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
