#pragma once

#include "types/momentary_switch_event_details/pointing_button.hpp"
#include <cstdint>
#include <pqrs/hid.hpp>
#include <pqrs/json.hpp>

namespace krbn::core_configuration::details {
// Routes one HID++ control of a device to a pointing button.
//
// {
//     "control_id": 196,
//     "pointing_button": "button17"
// }
class hidpp_button final {
public:
  hidpp_button(uint16_t control_id,
               pqrs::hid::usage::value_t pointing_button)
      : control_id_(control_id),
        pointing_button_(pointing_button) {
  }

  explicit hidpp_button(const nlohmann::json& json)
      : control_id_(0),
        pointing_button_(pqrs::hid::usage::value_t(0)) {
    pqrs::json::requires_object(json, "json");

    bool control_id_found = false;
    bool pointing_button_found = false;

    for (const auto& [key, value] : json.items()) {
      if (key == "control_id") {
        if (!value.is_number_integer() ||
            value.get<int64_t>() < 1 ||
            value.get<int64_t>() > 0xffff) {
          throw pqrs::json::unmarshal_error(fmt::format("`control_id` must be an integer between 1 and 65535, but is `{0}`",
                                                        pqrs::json::dump_for_error_message(value)));
        }
        control_id_ = static_cast<uint16_t>(value.get<int64_t>());
        control_id_found = true;

      } else if (key == "pointing_button") {
        pqrs::json::requires_string(value, "`pointing_button`");
        pointing_button_ = momentary_switch_event_details::pointing_button::make_usage_pair(key, value).get_usage();
        pointing_button_found = true;

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                      key,
                                                      pqrs::json::dump_for_error_message(json)));
      }
    }

    if (!control_id_found) {
      throw pqrs::json::unmarshal_error(fmt::format("`control_id` is missing in `{0}`",
                                                    pqrs::json::dump_for_error_message(json)));
    }
    if (!pointing_button_found) {
      throw pqrs::json::unmarshal_error(fmt::format("`pointing_button` is missing in `{0}`",
                                                    pqrs::json::dump_for_error_message(json)));
    }
  }

  [[nodiscard]] nlohmann::json to_json() const {
    return nlohmann::json::object({
        {"control_id", control_id_},
        {"pointing_button", momentary_switch_event_details::pointing_button::make_name(pointing_button_)},
    });
  }

  [[nodiscard]] uint16_t get_control_id() const {
    return control_id_;
  }

  [[nodiscard]] pqrs::hid::usage::value_t get_pointing_button() const {
    return pointing_button_;
  }

  bool operator==(const hidpp_button&) const = default;

private:
  uint16_t control_id_;
  pqrs::hid::usage::value_t pointing_button_;
};
} // namespace krbn::core_configuration::details
