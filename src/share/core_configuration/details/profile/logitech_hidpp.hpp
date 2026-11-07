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
    bool enabled{true};

    bool operator==(const button&) const = default;
  };

  explicit logitech_hidpp(std::vector<button> buttons)
      : buttons_(normalize(std::move(buttons))) {
  }

  explicit logitech_hidpp(const nlohmann::json& json) {
    pqrs::json::requires_object(json, "json");

    const auto it = json.find("buttons");
    if (it != std::end(json)) {
      pqrs::json::requires_array(*it, "`buttons`");
    }

    if (it != std::end(json)) {
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
    }

    buttons_ = normalize(std::move(buttons_));
  }

  [[nodiscard]] nlohmann::json to_json() const {
    if (buttons_.empty()) {
      return nlohmann::json::object();
    }

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
    std::vector<button> result;
    for (auto&& button : value) {
      if (std::ranges::none_of(result,
                               [&](const auto& existing) {
                                 return existing.control_id == button.control_id;
                               })) {
        result.push_back(std::move(button));
      }
    }
    std::ranges::sort(result, {}, &button::control_id);
    return result;
  }

  std::vector<button> buttons_;
};
} // namespace krbn::core_configuration::details
