#pragma once

#include "core_configuration.hpp"
#include "manipulator/details/base.hpp"
#include "manipulator/details/basic.hpp"
#include "manipulator/details/conditions/device.hpp"
#include "manipulator/details/conditions/frontmost_application.hpp"
#include "manipulator/details/conditions/input_source.hpp"
#include "manipulator/details/conditions/nop.hpp"
#include "manipulator/details/conditions/variable.hpp"
#include "manipulator/details/nop.hpp"
#include "manipulator/details/types.hpp"
#include <memory>

namespace krbn {
namespace manipulator {
class manipulator_factory final {
public:
  static std::shared_ptr<details::base> make_manipulator(const nlohmann::json& json,
                                                         const core_configuration::profile::complex_modifications::parameters& parameters) {
    {
      const std::string key = "type";
      if (json.find(key) != std::end(json) && json[key].is_string()) {
        const std::string& value = json[key];
        if (value == "basic") {
          return std::make_shared<details::basic>(json, parameters);
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown `type` {0} in {1}", value, json.dump());
          return std::make_shared<details::nop>();
        }
      }
    }
    logger::get_logger().error("complex_modifications json error: `type` is not found in {0}", json.dump());
    return std::make_shared<details::nop>();
  }

  static std::shared_ptr<details::conditions::base> make_condition(const nlohmann::json& json) {
    {
      const std::string key = "type";
      if (json.find(key) != std::end(json) && json[key].is_string()) {
        const std::string& value = json[key];
        if (value == "device_if" ||
            value == "device_unless") {
          return std::make_shared<details::conditions::device>(json);
        } else if (value == "frontmost_application_if" ||
                   value == "frontmost_application_unless") {
          return std::make_shared<details::conditions::frontmost_application>(json);
        } else if (value == "input_source_if" ||
                   value == "input_source_unless") {
          return std::make_shared<details::conditions::input_source>(json);
        } else if (value == "variable_if" ||
                   value == "variable_unless") {
          return std::make_shared<details::conditions::variable>(json);
        } else {
          logger::get_logger().error("complex_modifications json error: unknown `type` {0} in {1}", value, json.dump());
          return std::make_shared<details::conditions::nop>();
        }
      }
    }
    logger::get_logger().error("complex_modifications json error: `type` is not found in {0}", json.dump());
    return std::make_shared<details::conditions::nop>();
  }
};
} // namespace manipulator
} // namespace krbn
