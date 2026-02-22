#pragma once

#include "conditions/base.hpp"

namespace krbn {
namespace manipulator {
class condition_manager final {
public:
  condition_manager(const condition_manager&) = delete;

  condition_manager(void) {
  }

  const std::vector<pqrs::not_null_shared_ptr_t<manipulator::conditions::base>>& get_conditions(void) const {
    return conditions_;
  }

  void push_back_condition(pqrs::not_null_shared_ptr_t<manipulator::conditions::base> condition) {
    conditions_.push_back(condition);
  }

  bool is_fulfilled(const manipulator::conditions::condition_context& condition_context,
                    manipulator_environment& manipulator_environment) const {
    bool result = true;

    //
    // Update condition expression variables
    //

    manipulator_environment.set_variable_system_now_milliseconds();
    get_shared_condition_expression_manager()->apply_environment_variables(manipulator_environment);

    //
    // Evaluate condition rules.
    //

    for (const auto& c : conditions_) {
      if (!c->is_fulfilled(condition_context,
                           manipulator_environment)) {
        result = false;
      }
    }

    return result;
  }

private:
  std::vector<pqrs::not_null_shared_ptr_t<manipulator::conditions::base>> conditions_;
};
} // namespace manipulator
} // namespace krbn
