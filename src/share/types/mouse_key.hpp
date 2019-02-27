#pragma once

#include "logger.hpp"
#include <cstdint>
#include <ostream>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
class mouse_key final {
public:
  mouse_key(void) : x_(0),
                    y_(0),
                    vertical_wheel_(0),
                    horizontal_wheel_(0),
                    speed_multiplier_(1.0) {
  }

  mouse_key(int x,
            int y,
            int vertical_wheel,
            int horizontal_wheel,
            double speed_multiplier) : x_(x),
                                       y_(y),
                                       vertical_wheel_(vertical_wheel),
                                       horizontal_wheel_(horizontal_wheel),
                                       speed_multiplier_(speed_multiplier) {
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

  int get_vertical_wheel(void) const {
    return vertical_wheel_;
  }

  void set_vertical_wheel(int value) {
    vertical_wheel_ = value;
  }

  int get_horizontal_wheel(void) const {
    return horizontal_wheel_;
  }

  void set_horizontal_wheel(int value) {
    horizontal_wheel_ = value;
  }

  double get_speed_multiplier(void) const {
    return speed_multiplier_;
  }

  void set_speed_multiplier(int value) {
    speed_multiplier_ = value;
  }

  bool is_zero(void) const {
    // Do not check speed_multiplier_ here.

    return x_ == 0 &&
           y_ == 0 &&
           vertical_wheel_ == 0 &&
           horizontal_wheel_ == 0;
  }

  void invert_wheel(void) {
    vertical_wheel_ = -vertical_wheel_;
    horizontal_wheel_ = -horizontal_wheel_;
  }

  mouse_key& operator+=(const mouse_key& other) {
    x_ += other.x_;
    y_ += other.y_;
    vertical_wheel_ += other.vertical_wheel_;
    horizontal_wheel_ += other.horizontal_wheel_;

    // multiply speed_multiplier_.
    speed_multiplier_ *= other.speed_multiplier_;

    return *this;
  }

  const mouse_key operator+(const mouse_key& other) const {
    mouse_key result = *this;
    result += other;
    return result;
  }

  bool operator==(const mouse_key& other) const {
    return x_ == other.x_ &&
           y_ == other.y_ &&
           vertical_wheel_ == other.vertical_wheel_ &&
           horizontal_wheel_ == other.horizontal_wheel_ &&
           speed_multiplier_ == other.speed_multiplier_;
  }

  bool operator!=(const mouse_key& other) const {
    return !(*this == other);
  }

private:
  int x_;
  int y_;
  int vertical_wheel_;
  int horizontal_wheel_;
  double speed_multiplier_;
};

inline void to_json(nlohmann::json& json, const mouse_key& m) {
  json["x"] = m.get_x();
  json["y"] = m.get_y();
  json["vertical_wheel"] = m.get_vertical_wheel();
  json["horizontal_wheel"] = m.get_horizontal_wheel();
  json["speed_multiplier"] = m.get_speed_multiplier();
}

inline void from_json(const nlohmann::json& json, mouse_key& m) {
  if (!json.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
  }

  for (const auto& [key, value] : json.items()) {
    if (key == "x") {
      if (!value.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
      }

      m.set_x(value.get<int>());

    } else if (key == "y") {
      if (!value.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
      }

      m.set_y(value.get<int>());

    } else if (key == "vertical_wheel") {
      if (!value.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
      }

      m.set_vertical_wheel(value.get<int>());

    } else if (key == "horizontal_wheel") {
      if (!value.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
      }

      m.set_horizontal_wheel(value.get<int>());

    } else if (key == "speed_multiplier") {
      if (!value.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
      }

      m.set_speed_multiplier(value.get<double>());

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", key));
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::mouse_key> final {
  std::size_t operator()(const krbn::mouse_key& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.get_x());
    pqrs::hash_combine(h, value.get_y());
    pqrs::hash_combine(h, value.get_vertical_wheel());
    pqrs::hash_combine(h, value.get_horizontal_wheel());
    pqrs::hash_combine(h, value.get_speed_multiplier());

    return h;
  }
};
} // namespace std
