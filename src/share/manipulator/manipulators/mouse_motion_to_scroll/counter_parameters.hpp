#pragma once

#include "types/absolute_time_duration.hpp"
#include <pqrs/osx/chrono.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
struct counter_parameters final {
public:
  counter_parameters(void) : threshold(32),
                             recent_time_duration(
                                 pqrs::osx::chrono::make_absolute_time_duration(
                                     std::chrono::milliseconds(100))),
                             value_scale(1.0),
                             momentum_max_count(100),
                             momentum_minus(16) {
  }

  int threshold;
  absolute_time_duration recent_time_duration;
  double value_scale;
  int momentum_max_count;
  int momentum_minus;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
