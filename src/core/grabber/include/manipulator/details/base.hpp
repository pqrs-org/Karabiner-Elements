#pragma once

#include "event_queue.hpp"

namespace krbn {
namespace manipulator {
namespace detail {
class base {
public:
  virtual ~base(void) {
  }

  virtual manipulate(event_queue& event_queue) = 0;
};
} // namespace detail
} // namespace manipulator
} // namespace krbn
