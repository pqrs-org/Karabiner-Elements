#pragma once

#include "conditions/base.hpp"

namespace krbn {
namespace manipulator {
class condition_manager final {
public:
  condition_manager(const condition_manager&) = delete;

  condition_manager(void) {
  }

  void push_back_condition(std::shared_ptr<manipulator::conditions::base> condition) {
    conditions_.push_back(condition);
  }

  bool is_fulfilled(const event_queue::entry& entry,
                    const manipulator_environment& manipulator_environment) const {
    bool result = true;

    for (const auto& c : conditions_) {
      if (!c->is_fulfilled(entry,
                           manipulator_environment)) {
        result = false;
      }
    }

    return result;
  }

private:
  std::vector<std::shared_ptr<manipulator::conditions::base>> conditions_;
};
} // namespace manipulator
} // namespace krbn
