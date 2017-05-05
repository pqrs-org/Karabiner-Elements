#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual void manipulate(event_queue& event_queue,
                          uint64_t time_stamp) {
  }

  virtual bool active(void) const {
    return false;
  }

  virtual void inactivate_by_device_id(event_queue& event_queue,
                                       device_id device_id,
                                       uint64_t time_stamp) {
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
