#pragma once

#include "types/absolute_time_point.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class counter_entry final {
public:
  counter_entry(int x,
                int y,
                pqrs::dispatcher::time_point time_point) : x_(x),
                                                           y_(y),
                                                           time_point_(time_point) {
  }

  int get_x(void) const {
    return x_;
  }

  int get_y(void) const {
    return y_;
  }

  pqrs::dispatcher::time_point get_time_point(void) const {
    return time_point_;
  }

private:
  int x_;
  int y_;
  pqrs::dispatcher::time_point time_point_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
