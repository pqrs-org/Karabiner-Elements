#pragma once

#include "software_function_details/cg_event_double_click.hpp"
#include "software_function_details/iokit_power_management_sleep_system.hpp"
#include "software_function_details/open_application.hpp"
#include "software_function_details/set_mouse_cursor_position.hpp"
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
class software_function final {
public:
  using value_t = std::variant<software_function_details::cg_event_double_click,
                               software_function_details::iokit_power_management_sleep_system,
                               software_function_details::open_application,
                               software_function_details::set_mouse_cursor_position,
                               std::monostate>;

  software_function(void) : value_(std::monostate()) {
  }

  const value_t& get_value(void) const {
    return value_;
  }

  void set_value(const value_t& value) {
    value_ = value;
  }

  template <typename T>
  const T* get_if(void) const {
    return std::get_if<T>(&value_);
  }

  constexpr bool operator==(const software_function&) const = default;

private:
  value_t value_;
};

inline void to_json(nlohmann::json& json, const software_function& value) {
  if (value.get_if<std::monostate>()) {
    json = nullptr;
  } else if (auto v = value.get_if<software_function_details::cg_event_double_click>()) {
    json = nlohmann::json::object({{"cg_event_double_click", *v}});
  } else if (auto v = value.get_if<software_function_details::iokit_power_management_sleep_system>()) {
    json = nlohmann::json::object({{"iokit_power_management_sleep_system", *v}});
  } else if (auto v = value.get_if<software_function_details::open_application>()) {
    json = nlohmann::json::object({{"open_application", *v}});
  } else if (auto v = value.get_if<software_function_details::set_mouse_cursor_position>()) {
    json = nlohmann::json::object({{"set_mouse_cursor_position", *v}});
  }
}

inline void from_json(const nlohmann::json& json, software_function& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "cg_event_double_click") {
      try {
        value.set_value(v.get<software_function_details::cg_event_double_click>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else if (k == "iokit_power_management_sleep_system") {
      try {
        value.set_value(v.get<software_function_details::iokit_power_management_sleep_system>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else if (k == "open_application") {
      try {
        value.set_value(v.get<software_function_details::open_application>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else if (k == "set_mouse_cursor_position") {
      try {
        value.set_value(v.get<software_function_details::set_mouse_cursor_position>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::software_function> final {
  std::size_t operator()(const krbn::software_function& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_value());

    return h;
  }
};
} // namespace std
