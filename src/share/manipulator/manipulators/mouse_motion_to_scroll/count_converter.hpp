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
                                                  std::end(recent_values_buffer_)) {
  }

  uint8_t update(int value) {
    recent_values_.push_back(value);

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

    return static_cast<uint8_t>(result);
  }

  int make_momentum_value(void) {
    if (recent_values_.empty()) {
      return 0;
    }

    auto total = std::accumulate(std::begin(recent_values_),
                                 std::end(recent_values_),
                                 0);
    return total / recent_values_.size() / 2;
  }

  void reset(void) {
    count_ = 0;
    while (!recent_values_.empty()) {
      recent_values_.pop_front();
    }
  }

private:
  int threshold_;
  int count_;
  std::vector<int> recent_values_buffer_;
  nonstd::ring_span<int> recent_values_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
