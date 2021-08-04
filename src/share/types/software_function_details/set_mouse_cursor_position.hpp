#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
namespace software_function_details {
class set_mouse_cursor_position {
public:
  set_mouse_cursor_position(void)
      : x_(0),
        y_(0) {
  }

  int get_x(void) const {
    return x_;
  }

  void set_x(int value) {
    x_ = value;
  }

  int get_y(void) const {
    return y_;
  }

  void set_y(int value) {
    y_ = value;
  }

  std::optional<uint32_t> get_screen(void) const {
    return screen_;
  }

  void set_screen(std::optional<uint32_t> value) {
    screen_ = value;
  }

  CGPoint get_point(const CGRect& bounds) const {
    return CGPointMake(x_, y_);
  }

  constexpr bool operator==(const set_mouse_cursor_position&) const = default;

private:
  int x_;
  int y_;
  std::optional<uint32_t> screen_;
};

inline void to_json(nlohmann::json& json, const set_mouse_cursor_position& value) {
  json["x"] = value.get_x();
  json["y"] = value.get_y();
  if (auto v = value.get_screen()) {
    json["screen"] = *v;
  }
}

inline void from_json(const nlohmann::json& json, set_mouse_cursor_position& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "x") {
      pqrs::json::requires_number(v, "`" + k + "`");
      value.set_x(v.get<int>());
    } else if (k == "y") {
      pqrs::json::requires_number(v, "`" + k + "`");
      value.set_y(v.get<int>());
    } else if (k == "screen") {
      pqrs::json::requires_number(v, "`" + k + "`");
      value.set_screen(v.get<int>());
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace software_function_details
} // namespace krbn

namespace std {
template <>
struct hash<krbn::software_function_details::set_mouse_cursor_position> final {
  std::size_t operator()(const krbn::software_function_details::set_mouse_cursor_position& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_x());
    pqrs::hash::combine(h, value.get_y());
    pqrs::hash::combine(h, value.get_screen());

    return h;
  }
};
} // namespace std
