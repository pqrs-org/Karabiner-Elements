#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_environment.hpp"

namespace krbn {
namespace manipulator {
namespace conditions {
struct condition_context final {
  device_id device_id;
  event_queue::state state;
};

class base {
protected:
  base(void) {
  }

public:
  virtual ~base(void) {
  }

  virtual bool is_fulfilled(const condition_context& condition_context,
                            const manipulator_environment& manipulator_environment) const = 0;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
