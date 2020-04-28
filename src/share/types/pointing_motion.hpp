#pragma once

#include <cstdint>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

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

inline void to_json(nlohmann::json& j, const pointing_motion& value) {
  j["x"] = value.get_x();
  j["y"] = value.get_y();
  j["vertical_wheel"] = value.get_vertical_wheel();
  j["horizontal_wheel"] = value.get_horizontal_wheel();
}

inline void from_json(const nlohmann::json& j, pointing_motion& value) {
  pqrs::json::requires_object(j, "json");

  for (const auto& [k, v] : j.items()) {
    if (k == "x") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_x(v.get<int>());
    }
    if (k == "y") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_y(v.get<int>());
    }
    if (k == "vertical_wheel") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_vertical_wheel(v.get<int>());
    }
    if (k == "horizontal_wheel") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_horizontal_wheel(v.get<int>());
    }
  }
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
