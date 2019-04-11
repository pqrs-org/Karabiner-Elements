#pragma once

#include "types/absolute_time_duration.hpp"
#include <pqrs/osx/chrono.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
struct counter_parameters final {
public:
  counter_parameters(void) : recent_time_duration(
                                 pqrs::osx::chrono::make_absolute_time_duration(
                                     std::chrono::milliseconds(100))),
                             value_scale(1.0),
                             momentum_minus(16),
                             threshold_(32) {
  }

  int get_threshold(void) const {
    return threshold_;
  }

  void set_threshold(int value) {
    if (value <= 0) {
      threshold_ = 1;
    } else {
      threshold_ = value;
    }
  }

  absolute_time_duration recent_time_duration;
  double value_scale;
  int momentum_minus;

private:
  int threshold_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
