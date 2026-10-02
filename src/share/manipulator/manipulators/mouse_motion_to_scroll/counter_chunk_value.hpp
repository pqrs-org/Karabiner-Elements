#pragma once

#include <cmath>
#include <cstdlib>
#include <pqrs/sign.hpp>

namespace krbn::manipulator::manipulators::mouse_motion_to_scroll {
class counter_chunk_value final {
public:
  [[nodiscard]] int get_abs_total() const {
    return abs_total_;
  }

  void add(int value) {
    if (value > 0) {
      plus_value_ += value;
      last_sign_ = pqrs::sign::plus;
    } else if (value < 0) {
      minus_value_ += value;
      last_sign_ = pqrs::sign::minus;
    }
    abs_total_ += std::abs(value);
  }

  [[nodiscard]] int make_accumulated_value() const {
    auto abs_p = std::abs(plus_value_);
    auto abs_m = std::abs(minus_value_);

    if (abs_p > abs_m) {
      return plus_value_;
    } else if (abs_p < abs_m) {
      return minus_value_;
    } else {
      switch (last_sign_) {
        case pqrs::sign::zero:
          return 0;
        case pqrs::sign::plus:
          return plus_value_;
        case pqrs::sign::minus:
          return minus_value_;
      }
    }
  }

private:
  int plus_value_{0};
  int minus_value_{0};
  int abs_total_{0};
  pqrs::sign last_sign_{pqrs::sign::zero};
};
} // namespace krbn::manipulator::manipulators::mouse_motion_to_scroll
