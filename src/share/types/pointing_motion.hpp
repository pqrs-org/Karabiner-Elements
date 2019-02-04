#pragma once

#include "stream_utility.hpp"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <pqrs/hash.hpp>

namespace krbn {
class pointing_motion final {
public:
  pointing_motion(void) : x_(0),
                          y_(0),
                          vertical_wheel_(0),
                          horizontal_wheel_(0) {
  }

  pointing_motion(int x,
                  int y,
                  int vertical_wheel,
                  int horizontal_wheel) : x_(x),
                                          y_(y),
                                          vertical_wheel_(vertical_wheel),
                                          horizontal_wheel_(horizontal_wheel) {
  }

  pointing_motion(const nlohmann::json& json) : pointing_motion() {
    if (auto v = json_utility::find_optional<int>(json, "x")) {
      x_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "y")) {
      y_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "vertical_wheel")) {
      vertical_wheel_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "horizontal_wheel")) {
      horizontal_wheel_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["x"] = x_;
    j["y"] = y_;
    j["vertical_wheel"] = vertical_wheel_;
    j["horizontal_wheel"] = horizontal_wheel_;
    return j;
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

  bool is_zero(void) const {
    return x_ == 0 &&
           y_ == 0 &&
           vertical_wheel_ == 0 &&
           horizontal_wheel_ == 0;
  }

  void clear(void) {
    x_ = 0;
    y_ = 0;
    vertical_wheel_ = 0;
    horizontal_wheel_ = 0;
  }

  bool operator==(const pointing_motion& other) const {
    return x_ == other.x_ &&
           y_ == other.y_ &&
           vertical_wheel_ == other.vertical_wheel_ &&
           horizontal_wheel_ == other.horizontal_wheel_;
  }

private:
  int x_;
  int y_;
  int vertical_wheel_;
  int horizontal_wheel_;
};

inline std::ostream& operator<<(std::ostream& stream, const pointing_motion& value) {
  stream << "pointing_motion:" << value.to_json();
  return stream;
}

inline void to_json(nlohmann::json& json, const pointing_motion& pointing_motion) {
  json = pointing_motion.to_json();
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::pointing_motion> final {
  std::size_t operator()(const krbn::pointing_motion& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.get_x());
    pqrs::hash_combine(h, value.get_y());
    pqrs::hash_combine(h, value.get_vertical_wheel());
    pqrs::hash_combine(h, value.get_horizontal_wheel());

    return h;
  }
};
} // namespace std
