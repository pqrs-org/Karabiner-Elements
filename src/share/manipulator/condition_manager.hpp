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

  bool is_fulfilled(const event_queue::entry& entry,
                    const manipulator_environment& manipulator_environment) const {
    bool result = true;

    //
    // Update condition expression variables
    //

    {
      auto m = get_shared_condition_expression_manager();
      auto c = manipulator_environment.get_core_configuration();
      if (auto dp = manipulator_environment.find_device_properties(entry.get_device_id())) {
        m->set_variable("device.vendor_id",
                        type_safe::get(dp->get_device_identifiers().get_vendor_id()));
        m->set_variable("device.product_id",
                        type_safe::get(dp->get_device_identifiers().get_product_id()));
        m->set_variable("device.location_id",
                        type_safe::get(dp->get_location_id()));
        m->set_variable("device.device_address",
                        dp->get_device_identifiers().get_device_address());
        m->set_variable("device.is_keyboard",
                        dp->get_device_identifiers().get_is_keyboard());
        m->set_variable("device.is_pointing_device",
                        dp->get_device_identifiers().get_is_pointing_device());
        m->set_variable("device.is_game_pad",
                        dp->get_device_identifiers().get_is_game_pad());
        m->set_variable("device.is_consumer",
                        dp->get_device_identifiers().get_is_consumer());
        m->set_variable("device.is_touch_bar",
                        dp->get_is_built_in_touch_bar());
        m->set_variable("device.is_built_in_keyboard",
                        device_utility::determine_is_built_in_keyboard(*c, *dp));
      }
    }

    //
    // Evaluate condition rules.
    //

    for (const auto& c : conditions_) {
      if (!c->is_fulfilled(entry,
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
