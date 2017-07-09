#pragma once

#include "manipulator/manipulator_factory.hpp"

namespace krbn {
namespace manipulator {
class condition_manager final {
public:
  condition_manager(const condition_manager&) = delete;

  condition_manager(void) {
  }

  void push_back_condition(const nlohmann::json& json) {
    conditions_.push_back(manipulator_factory::make_condition(json));
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
