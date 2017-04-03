#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "manipulator/event_queue.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual void manipulate(event_queue& event_queue) {
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
