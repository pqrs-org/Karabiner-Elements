#pragma once

#include <nonstd/ring_span.hpp>
#include <numeric>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class count_converter final {
public:
  count_converter(int threshold) : threshold_(threshold),
                                   count_(0),
                                   recent_values_buffer_(8),
                                   recent_values_(std::begin(recent_values_buffer_),
                                                  std::end(recent_values_buffer_)),
                                   momentum_value_(0) {
  }

  int update(int value) {
    recent_values_.push_back(value);
    initialize_momentum_value();

    return add(value);
  }

  int update_momentum(void) {
    recent_values_.push_back(0);

    momentum_value_ = static_cast<int>(momentum_value_ * 0.8);

    return add(momentum_value_);
  }

  void reset(void) {
    count_ = 0;

    while (!recent_values_.empty()) {
      recent_values_.pop_front();
    }

    momentum_value_ = 0;
  }

private:
  int add(int value) {
    int result = 0;

    count_ += value;

    while (count_ <= -threshold_) {
      --result;
      count_ += threshold_;
    }
    while (count_ >= threshold_) {
      ++result;
      count_ -= threshold_;
    }

    return result;
  }

  void initialize_momentum_value(void) {
    if (recent_values_.empty()) {
      momentum_value_ = 0;

    } else {
      auto total = std::accumulate(std::begin(recent_values_),
                                   std::end(recent_values_),
                                   0);
      momentum_value_ = total / recent_values_.size();
    }
  }

  int threshold_;
  int count_;

  // `recent_values_` is used to calculate an initial momentum value.
  std::vector<int> recent_values_buffer_;
  nonstd::ring_span<int> recent_values_;

  int momentum_value_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
