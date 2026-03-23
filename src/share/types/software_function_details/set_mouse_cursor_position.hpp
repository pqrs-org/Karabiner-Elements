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

  enum class relative_to {
    screen,
    focused_window,
  };

  enum class fallback_to {
    none,
    screen,
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

  relative_to get_relative_to(void) const {
    return relative_to_;
  }

  void set_relative_to(relative_to value) {
    relative_to_ = value;
  }

  fallback_to get_fallback_to(void) const {
    return fallback_to_;
  }

  void set_fallback_to(fallback_to value) {
    fallback_to_ = value;
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
  relative_to relative_to_ = relative_to::screen;
  fallback_to fallback_to_ = fallback_to::none;
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    set_mouse_cursor_position::relative_to,
    {
        {set_mouse_cursor_position::relative_to::screen, "screen"},
        {set_mouse_cursor_position::relative_to::focused_window, "focused_window"},
    });

NLOHMANN_JSON_SERIALIZE_ENUM(
    set_mouse_cursor_position::fallback_to,
    {
        {set_mouse_cursor_position::fallback_to::none, "none"},
        {set_mouse_cursor_position::fallback_to::screen, "screen"},
    });

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
  if (value.get_relative_to() != set_mouse_cursor_position::relative_to::screen) {
    json["relative_to"] = value.get_relative_to();
  }
  if (value.get_fallback_to() != set_mouse_cursor_position::fallback_to::none) {
    json["fallback_to"] = value.get_fallback_to();
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
    } else if (k == "relative_to") {
      pqrs::json::requires_string(v, "`" + k + "`");
      auto s = v.get<std::string>();

      if (s == "screen") {
        value.set_relative_to(set_mouse_cursor_position::relative_to::screen);
      } else if (s == "focused_window") {
        value.set_relative_to(set_mouse_cursor_position::relative_to::focused_window);
      } else {
        throw pqrs::json::unmarshal_error("unknown relative_to: " + s);
      }
    } else if (k == "fallback_to") {
      pqrs::json::requires_string(v, "`" + k + "`");
      auto s = v.get<std::string>();

      if (s == "none") {
        value.set_fallback_to(set_mouse_cursor_position::fallback_to::none);
      } else if (s == "screen") {
        value.set_fallback_to(set_mouse_cursor_position::fallback_to::screen);
      } else {
        throw pqrs::json::unmarshal_error("unknown fallback_to: " + s);
      }
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
    pqrs::hash::combine(h, value.get_relative_to());
    pqrs::hash::combine(h, value.get_fallback_to());

    return h;
  }
};
} // namespace std
