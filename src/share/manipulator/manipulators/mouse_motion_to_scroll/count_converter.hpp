#pragma once

#include <algorithm>
#include <numeric>
#include <optional>
#include <pqrs/sign.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class count_converter final {
public:
  count_converter(int threshold) : threshold_(threshold),
                                   sign_(pqrs::sign::zero),
                                   count_(0),
                                   over_threshold_(false),
                                   momentum_value_(0),
                                   momentum_count_(0) {
  }

  int get_count(void) const {
    return count_;
  }

  int update(int value) {
    // Reset if sign is changed.

    if (sign_ != pqrs::sign::zero) {
      auto s = pqrs::make_sign(value);
      if (sign_ != s) {
        reset();
      }
    }

    if (sign_ == pqrs::sign::zero) {
      sign_ = pqrs::make_sign(value);
    }

    if (!over_threshold_) {
      value *= threshold_ / 4;
    }

    auto result = add(value);

    momentum_count_ = 0;
    momentum_value_ += (result * threshold_ / 4);

    return result;
  }

  std::optional<int> update_momentum(void) {
    if (momentum_value_ > 1) {
      --momentum_value_;
    } else if (momentum_value_ < -1) {
      ++momentum_value_;
    }

    ++momentum_count_;
    if (momentum_count_ > 10) {
      momentum_value_ = 0;
    }

    if (momentum_value_ == 0) {
      return std::nullopt;
    }

    return add(momentum_value_);
  }

  void reset(void) {
    sign_ = pqrs::sign::zero;
    count_ = 0;
    over_threshold_ = false;
    momentum_value_ = 0;
  }

private:
  int add(int value) {
    int result = 0;

    count_ += value;

    while (count_ <= -threshold_) {
      --result;
      count_ += threshold_;
      over_threshold_ = true;
    }
    while (count_ >= threshold_) {
      ++result;
      count_ -= threshold_;
      over_threshold_ = true;
    }

    return result;
  }

  int threshold_;

  pqrs::sign sign_;
  int count_;
  bool over_threshold_;

  int momentum_value_;
  int momentum_count_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
