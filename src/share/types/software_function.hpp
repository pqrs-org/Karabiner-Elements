#pragma once

#include "software_function_details/cg_event_double_click.hpp"
#include "software_function_details/set_mouse_cursor_position.hpp"
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
class software_function final {
public:
  using value_t = std::variant<software_function_details::cg_event_double_click,
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

private:
  value_t value_;
};

inline void to_json(nlohmann::json& json, const software_function& value) {
  if (auto v = value.get_if<std::monostate>()) {
    json = nullptr;
  } else if (auto v = value.get_if<software_function_details::cg_event_double_click>()) {
    json = *v;
  } else if (auto v = value.get_if<software_function_details::set_mouse_cursor_position>()) {
    json = *v;
  }
}

inline void from_json(const nlohmann::json& json, software_function& value) {
  pqrs::json::requires_object(json, "json");

  auto type = pqrs::json::find<std::string>(json, "type");
  if (!type) {
    throw pqrs::json::unmarshal_error(fmt::format("`type` must be specified: {0}", pqrs::json::dump_for_error_message(json)));
  }

  if (*type == software_function_details::cg_event_double_click::type) {
    value.set_value(json.get<software_function_details::cg_event_double_click>());
  } else if (*type == software_function_details::set_mouse_cursor_position::type) {
    value.set_value(json.get<software_function_details::set_mouse_cursor_position>());
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
