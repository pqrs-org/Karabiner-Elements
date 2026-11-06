#pragma once

#include "types/momentary_switch_event.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <pqrs/json.hpp>
#include <vector>

namespace krbn::core_configuration::details {
// Diverts HID++ buttons of a device and maps the resulting logical input
// events through the Simple Modifications manipulator manager.
//
// {
//     "buttons": [
//         {
//             "control_id": 82,
//             "enabled": true,
//             "to": [{ "key_code": "mission_control" }]
//         }
//     ]
// }
class logitech_hidpp final {
public:
  struct button final {
    uint16_t control_id;
    nlohmann::json to;
    bool enabled = true;

    bool operator==(const button&) const = default;
  };

  explicit logitech_hidpp(std::vector<button> buttons)
      : buttons_(normalize(std::move(buttons))) {
    validate_buttons();
  }

  explicit logitech_hidpp(const nlohmann::json& json) {
    pqrs::json::requires_object(json, "json");

    const auto it = json.find("buttons");
    if (it == std::end(json)) {
      throw pqrs::json::unmarshal_error(fmt::format("`buttons` is missing in `{0}`",
                                                    pqrs::json::dump_for_error_message(json)));
    }
    pqrs::json::requires_array(*it, "`buttons`");

    for (const auto& item : json.items()) {
      if (item.key() != "buttons") {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                      item.key(),
                                                      pqrs::json::dump_for_error_message(json)));
      }
    }

    for (const auto& v : *it) {
      pqrs::json::requires_object(v, "values in `buttons`");

      std::optional<uint16_t> control_id;
      std::optional<nlohmann::json> to;
      bool enabled = true;
      for (const auto& [key, value] : v.items()) {
        if (key == "control_id") {
          const auto event = nlohmann::json({{"logitech_hidpp_button", value}})
                                 .get<momentary_switch_event>();
          control_id = static_cast<uint16_t>(
              type_safe::get(event.get_usage_pair().get_usage()));
        } else if (key == "to") {
          pqrs::json::requires_array(value, "`to`");
          to = value;
        } else if (key == "enabled") {
          pqrs::json::requires_boolean(value, "`enabled`");
          enabled = value.get<bool>();
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                        key,
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
      buttons_.push_back({
          .control_id = *control_id,
          .to = std::move(*to),
          .enabled = enabled,
      });
    }

    buttons_ = normalize(std::move(buttons_));
    validate_buttons();
  }

  [[nodiscard]] nlohmann::json to_json() const {
    auto buttons = nlohmann::json::array();
    for (const auto& button : buttons_) {
      buttons.push_back({
          {"control_id", button.control_id},
          {"enabled", button.enabled},
          {"to", button.to},
      });
    }
    return nlohmann::json::object({
        {"buttons", std::move(buttons)},
    });
  }

  [[nodiscard]] const std::vector<button>& get_buttons() const {
    return buttons_;
  }

  [[nodiscard]] std::vector<uint16_t> get_enabled_control_ids() const {
    std::vector<uint16_t> result;
    for (const auto& button : buttons_) {
      if (button.enabled) {
        result.push_back(button.control_id);
      }
    }
    return result;
  }

  [[nodiscard]] bool is_enabled(uint16_t control_id) const {
    return std::ranges::any_of(buttons_,
                               [control_id](const auto& button) {
                                 return button.control_id == control_id &&
                                        button.enabled;
                               });
  }

  bool operator==(const logitech_hidpp&) const = default;

private:
  [[nodiscard]] static std::vector<button> normalize(std::vector<button> value) {
    std::ranges::sort(value, {}, &button::control_id);
    return value;
  }

  void validate_buttons() const {
    if (buttons_.empty()) {
      throw pqrs::json::unmarshal_error("`buttons` must not be empty");
    }

    std::optional<uint16_t> previous_control_id;
    for (const auto& button : buttons_) {
      if (previous_control_id == button.control_id) {
        throw pqrs::json::unmarshal_error(fmt::format("`control_id` {0} is duplicated", button.control_id));
      }
      if (button.to.empty()) {
        throw pqrs::json::unmarshal_error(fmt::format("`to` of `control_id` {0} must not be empty", button.control_id));
      }
      previous_control_id = button.control_id;
    }
  }

  std::vector<button> buttons_;
};
} // namespace krbn::core_configuration::details
