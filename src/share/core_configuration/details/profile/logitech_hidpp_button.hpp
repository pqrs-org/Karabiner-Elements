#pragma once

#include <cstdint>
#include <pqrs/json.hpp>

namespace krbn::core_configuration::details {
// Diverts one HID++ control of a device and reports it as a logical
// `logitech_hidpp_button` input event.
//
// {
//     "control_id": 196
// }
class logitech_hidpp_button final {
public:
  explicit logitech_hidpp_button(uint16_t control_id)
      : control_id_(control_id) {
  }

  explicit logitech_hidpp_button(const nlohmann::json& json)
      : control_id_(0) {
    pqrs::json::requires_object(json, "json");

    bool control_id_found = false;
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
  }

  [[nodiscard]] nlohmann::json to_json() const {
    return nlohmann::json::object({
        {"control_id", control_id_},
    });
  }

  [[nodiscard]] uint16_t get_control_id() const {
    return control_id_;
  }

  bool operator==(const logitech_hidpp_button&) const = default;

private:
  uint16_t control_id_;
};
} // namespace krbn::core_configuration::details
