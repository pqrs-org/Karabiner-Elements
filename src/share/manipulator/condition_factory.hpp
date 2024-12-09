#pragma once

#include "conditions/device.hpp"
#include "conditions/event_changed.hpp"
#include "conditions/frontmost_application.hpp"
#include "conditions/input_source.hpp"
#include "conditions/keyboard_type.hpp"
#include "conditions/nop.hpp"
#include "conditions/variable.hpp"
#include "core_configuration/core_configuration.hpp"
#include <memory>

namespace krbn {
namespace manipulator {
namespace condition_factory {

inline gsl::not_null<std::shared_ptr<conditions::base>> make_condition(const nlohmann::json& json) {
  auto it = json.find("type");
  if (it == std::end(json)) {
    throw pqrs::json::unmarshal_error(
        fmt::format("condition type is not specified in `{0}`", pqrs::json::dump_for_error_message(json)));
  }

  pqrs::json::requires_string(*it, "condition type");

  auto type = it->get<std::string>();

  if (type == "device_if" ||
      type == "device_unless" ||
      type == "device_exists_if" ||
      type == "device_exists_unless") {
    return std::make_shared<conditions::device>(json);
  } else if (type == "event_changed_if" ||
             type == "event_changed_unless") {
    return std::make_shared<conditions::event_changed>(json);
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
        fmt::format("unknown condition type `{0}` in `{1}`", type, pqrs::json::dump_for_error_message(json)));
  }
}

inline gsl::not_null<std::shared_ptr<conditions::base>> make_device_if_condition(const core_configuration::details::device& device) {
  nlohmann::json json;
  json["type"] = "device_if";
  json["identifiers"] = nlohmann::json::array({
      nlohmann::json::object({
          {"vendor_id", type_safe::get(device.get_identifiers().get_vendor_id())},
          {"product_id", type_safe::get(device.get_identifiers().get_product_id())},
          {"is_keyboard", device.get_identifiers().get_is_keyboard()},
          {"is_pointing_device", device.get_identifiers().get_is_pointing_device()},
          // Skip location_id because it changes according to the location of the connected USB port.
          // Skip is_touch_bar in order to override by is_keyboard.
      }),
  });
  return std::make_shared<conditions::device>(json);
}

inline gsl::not_null<std::shared_ptr<conditions::base>> make_device_unless_touch_bar_condition(void) {
  nlohmann::json json;
  json["type"] = "device_unless";
  json["identifiers"] = nlohmann::json::array({
      nlohmann::json::object({
          {"is_touch_bar", true},
      }),
  });
  return std::make_shared<conditions::device>(json);
}

inline gsl::not_null<std::shared_ptr<conditions::base>> make_event_changed_if_condition(bool value) {
  nlohmann::json json;
  json["type"] = "event_changed_if";
  json["value"] = value;
  return std::make_shared<conditions::event_changed>(json);
}

inline gsl::not_null<std::shared_ptr<conditions::base>> make_frontmost_application_unless_condition(const std::vector<std::string>& bundle_identifiers) {
  nlohmann::json json;
  json["type"] = "frontmost_application_unless";
  json["bundle_identifiers"] = bundle_identifiers;
  return std::make_shared<conditions::frontmost_application>(json);
}

} // namespace condition_factory
} // namespace manipulator
} // namespace krbn
