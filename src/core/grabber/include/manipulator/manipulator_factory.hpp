#pragma once

#include "conditions/device.hpp"
#include "conditions/frontmost_application.hpp"
#include "conditions/input_source.hpp"
#include "conditions/keyboard_type.hpp"
#include "conditions/nop.hpp"
#include "conditions/variable.hpp"
#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulators/base.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/manipulators/nop.hpp"
#include "manipulator/types.hpp"
#include <memory>

namespace krbn {
namespace manipulator {
class manipulator_factory final {
public:
  static std::shared_ptr<manipulators::base> make_manipulator(const nlohmann::json& json,
                                                              const core_configuration::details::complex_modifications_parameters& parameters) {
    try {
      {
        if (auto value = json_utility::find_optional<std::string>(json, "type")) {
          if (*value == "basic") {
            return std::make_shared<manipulators::basic::basic>(json,
                                                                parameters);
          } else {
            logger::get_logger()->error("complex_modifications json error: Unknown `type` {0} in {1}", *value, json.dump());
            return std::make_shared<manipulators::nop>();
          }
        }
      }
      logger::get_logger()->error("complex_modifications json error: `type` is not found in {0}", json.dump());
      return std::make_shared<manipulators::nop>();

    } catch (std::exception& e) {
      logger::get_logger()->error("complex_modifications json error: {0}: {1}", e.what(), json.dump());
      return std::make_shared<manipulators::nop>();
    }
  }

  static std::shared_ptr<conditions::base> make_condition(const nlohmann::json& json) {
    auto it = json.find("type");
    if (it == std::end(json)) {
      throw pqrs::json::unmarshal_error(
          fmt::format("condition type is not specified in `{0}`", json.dump()));
    }

    if (!it->is_string()) {
      throw pqrs::json::unmarshal_error(
          fmt::format("condition type must be string, but is `{0}`", it->dump()));
    }

    auto type = it->get<std::string>();

    if (type == "device_if" ||
        type == "device_unless") {
      return std::make_shared<conditions::device>(json);
    } else if (type == "frontmost_application_if" ||
               type == "frontmost_application_unless") {
      return std::make_shared<conditions::frontmost_application>(json);
    } else if (type == "input_source_if" ||
               type == "input_source_unless") {
      return std::make_shared<conditions::input_source>(json);
    } else if (type == "variable_if" ||
               type == "variable_unless") {
      return std::make_shared<conditions::variable>(json);
    } else if (type == "keyboard_type_if" ||
               type == "keyboard_type_unless") {
      return std::make_shared<conditions::keyboard_type>(json);
    } else {
      throw pqrs::json::unmarshal_error(
          fmt::format("unknown condition type `{0}` in `{1}`", type, json.dump()));
    }
  }
};
} // namespace manipulator
} // namespace krbn
