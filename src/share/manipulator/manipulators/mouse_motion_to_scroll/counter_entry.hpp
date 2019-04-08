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
                absolute_time_point time_stamp) : x_(x),
                                                  y_(y),
                                                  time_stamp_(time_stamp) {
  }

  int get_x(void) const {
    return x_;
  }

  int get_y(void) const {
    return y_;
  }

  absolute_time_point get_time_stamp(void) const {
    return time_stamp_;
  }

private:
  int x_;
  int y_;
  absolute_time_point time_stamp_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
