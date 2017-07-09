#pragma once

#include "manipulator/details/conditions/base.hpp"

namespace krbn {
namespace manipulator {
class condition_manager final {
public:
  condition_manager(const condition_manager&) = delete;

  condition_manager(void) {
  }

  void push_back_condition(const std::shared_ptr<krbn::manipulator::details::conditions::base>& condition) {
    conditions_.push_back(condition);
  }

  bool is_fulfilled(const krbn::manipulator_environment& manipulator_environment) const {
    bool result = true;

    for (const auto& c : conditions_) {
      if (!c->is_fulfilled(manipulator_environment)) {
        result = false;
      }
    }

    return result;
  }

private:
  std::vector<std::shared_ptr<krbn::manipulator::details::conditions::base>> conditions_;
};
} // namespace manipulator
} // namespace krbn
