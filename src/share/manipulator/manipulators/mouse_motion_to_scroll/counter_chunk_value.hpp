#pragma once

#include <cmath>
#include <cstdlib>
#include <pqrs/sign.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class counter_chunk_value final {
public:
  counter_chunk_value(void) : plus_value_(0),
                              minus_value_(0),
                              abs_total_(0),
                              last_sign_(pqrs::sign::zero) {
  }

  int get_abs_total(void) const {
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

  int make_accumulated_value(void) const {
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
  int plus_value_;
  int minus_value_;
  int abs_total_;
  pqrs::sign last_sign_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
