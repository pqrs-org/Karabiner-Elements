#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <regex>

namespace krbn {
namespace software_function_details {
class set_mouse_cursor_position {
public:
  class position_value {
  public:
    enum class type {
      point,
      percent,
    };

    position_value(void)
        : value_(0),
          type_(type::point) {
    }

    int get_value(void) const {
      return value_;
    }

    void set_value(int value) {
      value_ = value;
    }

    type get_type(void) const {
      return type_;
    }

    void set_type(type value) {
      type_ = value;
    }

    int point_value(int bounds) const {
      switch (type_) {
        case type::point:
          return value_;
        case type::percent:
          return (bounds * value_) / 100;
      }
    }

    constexpr bool operator==(const position_value&) const = default;

  private:
    int value_;
    type type_;
  };

  set_mouse_cursor_position(void) {
  }

  const position_value& get_x(void) const {
    return x_;
  }

  void set_x(const position_value& value) {
    x_ = value;
  }

  const position_value& get_y(void) const {
    return y_;
  }

  void set_y(const position_value& value) {
    y_ = value;
  }

  std::optional<uint32_t> get_screen(void) const {
    return screen_;
  }

  void set_screen(std::optional<uint32_t> value) {
    screen_ = value;
  }

  CGPoint get_point(const CGRect& bounds) const {
    return CGPointMake(x_.point_value(bounds.size.width),
                       y_.point_value(bounds.size.height));
  }

  constexpr bool operator==(const set_mouse_cursor_position&) const = default;

private:
  position_value x_;
  position_value y_;
  std::optional<uint32_t> screen_;
};

//
// set_mouse_cursor_position::position_value json
//

inline void to_json(nlohmann::json& json, const set_mouse_cursor_position::position_value& value) {
  switch (value.get_type()) {
    case set_mouse_cursor_position::position_value::type::point:
      json = value.get_value();
      break;

    case set_mouse_cursor_position::position_value::type::percent:
      json = fmt::format("{0}%", value.get_value());
      break;
  }
}

inline void from_json(const nlohmann::json& json, set_mouse_cursor_position::position_value& value) {
  if (json.is_number()) {
    value.set_value(json.get<int>());
    value.set_type(set_mouse_cursor_position::position_value::type::point);
    return;
  }

  if (json.is_string()) {
    auto s = json.get<std::string>();

    try {
      //
      // percent
      //

      {
        std::regex r("(\\d+)%");
        std::smatch m;

        if (std::regex_match(s, m, r)) {
          value.set_value(std::stoi(m[1]));
          value.set_type(set_mouse_cursor_position::position_value::type::percent);
          return;
        }
      }

      //
      // point
      //

      value.set_value(std::stoi(s));
      value.set_type(set_mouse_cursor_position::position_value::type::point);
      return;

    } catch (std::exception& e) {
      throw pqrs::json::unmarshal_error(fmt::format("unsupported format: `{0}`", s));
    }
  }

  throw pqrs::json::unmarshal_error(fmt::format("json must be number or string, but is `{0}`", pqrs::json::dump_for_error_message(json)));
}

//
// set_mouse_cursor_position json
//

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
      value.set_x(v.get<set_mouse_cursor_position::position_value>());
    } else if (k == "y") {
      value.set_y(v.get<set_mouse_cursor_position::position_value>());
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
struct hash<krbn::software_function_details::set_mouse_cursor_position::position_value> final {
  std::size_t operator()(const krbn::software_function_details::set_mouse_cursor_position::position_value& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_value());
    pqrs::hash::combine(h, value.get_type());

    return h;
  }
};

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
