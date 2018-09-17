#pragma once

#include "boost_defs.hpp"

#include "logger.hpp"
#include <boost/functional/hash.hpp>
#include <cstdint>
#include <ostream>

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

  mouse_key(const nlohmann::json& json) : mouse_key() {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "x") {
          if (value.is_number()) {
            x_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.x should be number: {0}", json.dump());
          }
        } else if (key == "y") {
          if (value.is_number()) {
            y_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.y should be number: {0}", json.dump());
          }
        } else if (key == "vertical_wheel") {
          if (value.is_number()) {
            vertical_wheel_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.vertical_wheel should be number: {0}", json.dump());
          }
        } else if (key == "horizontal_wheel") {
          if (value.is_number()) {
            horizontal_wheel_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.horizontal_wheel should be number: {0}", json.dump());
          }
        } else if (key == "speed_multiplier") {
          if (value.is_number()) {
            speed_multiplier_ = value.get<double>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.speed_multiplier should be number: {0}", json.dump());
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["x"] = x_;
    j["y"] = y_;
    j["vertical_wheel"] = vertical_wheel_;
    j["horizontal_wheel"] = horizontal_wheel_;
    j["speed_multiplier"] = speed_multiplier_;
    return j;
  }

  int get_x(void) const {
    return x_;
  }

  int get_y(void) const {
    return y_;
  }

  int get_vertical_wheel(void) const {
    return vertical_wheel_;
  }

  int get_horizontal_wheel(void) const {
    return horizontal_wheel_;
  }

  double get_speed_multiplier(void) const {
    return speed_multiplier_;
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

  friend size_t hash_value(const mouse_key& value) {
    size_t h = 0;
    boost::hash_combine(h, value.x_);
    boost::hash_combine(h, value.y_);
    boost::hash_combine(h, value.vertical_wheel_);
    boost::hash_combine(h, value.horizontal_wheel_);
    boost::hash_combine(h, value.speed_multiplier_);
    return h;
  }

private:
  int x_;
  int y_;
  int vertical_wheel_;
  int horizontal_wheel_;
  double speed_multiplier_;
};
} // namespace krbn

namespace std {
template <>
struct hash<krbn::mouse_key> final {
  std::size_t operator()(const krbn::mouse_key& v) const {
    return hash_value(v);
  }
};
} // namespace std
