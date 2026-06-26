#pragma once

#include "types/absolute_time_point.hpp"

namespace krbn::manipulator::manipulators::mouse_motion_to_scroll {
class counter_entry final {
public:
  counter_entry(int x,
                int y,
                pqrs::dispatcher::time_point time_point) : x_(x),
                                                           y_(y),
                                                           time_point_(time_point) {
  }

  [[nodiscard]] int get_x() const {
    return x_;
  }

  [[nodiscard]] int get_y() const {
    return y_;
  }

  [[nodiscard]] pqrs::dispatcher::time_point get_time_point() const {
    return time_point_;
  }

private:
  int x_;
  int y_;
  pqrs::dispatcher::time_point time_point_;
};
} // namespace krbn::manipulator::manipulators::mouse_motion_to_scroll
