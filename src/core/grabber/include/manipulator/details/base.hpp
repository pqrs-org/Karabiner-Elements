#pragma once

#include "manipulator/event_queue.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class base {
public:
  virtual ~base(void) {
  }

  virtual void manipulate(event_queue& event_queue) = 0;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
