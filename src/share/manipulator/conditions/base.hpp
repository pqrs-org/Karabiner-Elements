#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_environment.hpp"

namespace krbn {
namespace manipulator {
namespace conditions {
class base {
protected:
  base(void) {
  }

public:
  virtual ~base(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const = 0;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
