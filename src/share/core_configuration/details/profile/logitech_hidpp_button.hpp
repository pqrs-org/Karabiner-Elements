#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <pqrs/json.hpp>
#include <vector>

namespace krbn::core_configuration::details {
// Diverts HID++ controls of a device and maps the resulting logical input
// events through the Simple Modifications manipulator manager.
//
// {
//     "controls": [
//         {
//             "control_id": 82,
//             "enabled": true,
//             "to": [{ "key_code": "mission_control" }]
//         }
//     ]
// }
class logitech_hidpp_button final {
public:
  struct control final {
    uint16_t control_id;
    nlohmann::json to;
    bool enabled = true;

    bool operator==(const control&) const = default;
  };

  explicit logitech_hidpp_button(std::vector<control> controls)
      : controls_(normalize(std::move(controls))) {
    validate_controls();
  }

  explicit logitech_hidpp_button(const nlohmann::json& json) {
    pqrs::json::requires_object(json, "json");

    bool controls_found = false;
    for (const auto& [key, value] : json.items()) {
      if (key == "controls") {
        pqrs::json::requires_array(value, "`controls`");
        for (const auto& v : value) {
          pqrs::json::requires_object(v, "values in `controls`");

          std::optional<uint16_t> control_id;
          std::optional<nlohmann::json> to;
          bool enabled = true;
          for (const auto& [control_key, control_value] : v.items()) {
            if (control_key == "control_id") {
              if (!control_value.is_number_integer() ||
                  control_value.get<int64_t>() < 1 ||
                  control_value.get<int64_t>() > 0xffff) {
                throw pqrs::json::unmarshal_error(fmt::format("`control_id` must be an integer between 1 and 65535, but is `{0}`",
                                                              pqrs::json::dump_for_error_message(control_value)));
              }
              control_id = static_cast<uint16_t>(control_value.get<int64_t>());
            } else if (control_key == "to") {
              pqrs::json::requires_array(control_value, "`to`");
              to = control_value;
            } else if (control_key == "enabled") {
              pqrs::json::requires_boolean(control_value, "`enabled`");
              enabled = control_value.get<bool>();
            } else {
              throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                            control_key,
                                                            pqrs::json::dump_for_error_message(v)));
            }
          }

          if (!control_id) {
            throw pqrs::json::unmarshal_error(fmt::format("`control_id` is missing in `{0}`",
                                                          pqrs::json::dump_for_error_message(v)));
          }
          if (!to) {
            throw pqrs::json::unmarshal_error(fmt::format("`to` is missing in `{0}`",
                                                          pqrs::json::dump_for_error_message(v)));
          }
          controls_.push_back({
              .control_id = *control_id,
              .to = std::move(*to),
              .enabled = enabled,
          });
        }
        controls_found = true;

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                      key,
                                                      pqrs::json::dump_for_error_message(json)));
      }
    }

    if (!controls_found) {
      throw pqrs::json::unmarshal_error(fmt::format("`controls` is missing in `{0}`",
                                                    pqrs::json::dump_for_error_message(json)));
    }
    controls_ = normalize(std::move(controls_));
    validate_controls();
  }

  [[nodiscard]] nlohmann::json to_json() const {
    auto controls = nlohmann::json::array();
    for (const auto& control : controls_) {
      controls.push_back({
          {"control_id", control.control_id},
          {"enabled", control.enabled},
          {"to", control.to},
      });
    }
    return nlohmann::json::object({
        {"controls", std::move(controls)},
    });
  }

  [[nodiscard]] const std::vector<control>& get_controls() const {
    return controls_;
  }

  [[nodiscard]] std::vector<uint16_t> get_enabled_control_ids() const {
    std::vector<uint16_t> result;
    for (const auto& control : controls_) {
      if (control.enabled) {
        result.push_back(control.control_id);
      }
    }
    return result;
  }

  [[nodiscard]] bool is_enabled(uint16_t control_id) const {
    return std::ranges::any_of(controls_,
                               [control_id](const auto& control) {
                                 return control.control_id == control_id &&
                                        control.enabled;
                               });
  }

  bool operator==(const logitech_hidpp_button&) const = default;

private:
  [[nodiscard]] static std::vector<control> normalize(std::vector<control> value) {
    std::ranges::sort(value, {}, &control::control_id);
    return value;
  }

  void validate_controls() const {
    if (controls_.empty()) {
      throw pqrs::json::unmarshal_error("`controls` must not be empty");
    }

    std::optional<uint16_t> previous_control_id;
    for (const auto& control : controls_) {
      if (previous_control_id == control.control_id) {
        throw pqrs::json::unmarshal_error(fmt::format("`control_id` {0} is duplicated", control.control_id));
      }
      if (control.to.empty()) {
        throw pqrs::json::unmarshal_error(fmt::format("`to` of `control_id` {0} must not be empty", control.control_id));
      }
      previous_control_id = control.control_id;
    }
  }

  std::vector<control> controls_;
};
} // namespace krbn::core_configuration::details
