#pragma once

#include "types/absolute_time_duration.hpp"
#include <pqrs/osx/chrono.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
struct counter_parameters final {
public:
  const double speed_multiplier_default_value = 1.0;
  const std::chrono::milliseconds recent_time_duration_milliseconds_default_value = std::chrono::milliseconds(100);
  const int threshold_default_value = 128;
  const int momentum_minus_default_value = 32;

  counter_parameters(void) : speed_multiplier_(speed_multiplier_default_value),
                             recent_time_duration_milliseconds_(recent_time_duration_milliseconds_default_value),
                             threshold_(threshold_default_value),
                             momentum_minus_(momentum_minus_default_value) {
  }

  double get_speed_multiplier(void) const {
    return speed_multiplier_;
  }

  void set_speed_multiplier(double value) {
    if (value <= 0.0) {
      value = speed_multiplier_default_value;
    }

    speed_multiplier_ = value;
  }

  std::chrono::milliseconds get_recent_time_duration_milliseconds(void) const {
    return recent_time_duration_milliseconds_;
  }

  void set_recent_time_duration_milliseconds(std::chrono::milliseconds value) {
    if (value < std::chrono::milliseconds(1)) {
      value = recent_time_duration_milliseconds_default_value;
    }

    recent_time_duration_milliseconds_ = value;
  }

  int get_threshold(void) const {
    return threshold_;
  }

  void set_threshold(int value) {
    if (value <= 0) {
      value = threshold_default_value;
    }

    threshold_ = value;
  }

  int get_momentum_minus(void) const {
    return momentum_minus_;
  }

  void set_momentum_minus(int value) {
    if (value <= 0) {
      value = momentum_minus_default_value;
    }

    momentum_minus_ = value;
  }

private:
  double speed_multiplier_;
  std::chrono::milliseconds recent_time_duration_milliseconds_;
  int threshold_;
  int momentum_minus_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
