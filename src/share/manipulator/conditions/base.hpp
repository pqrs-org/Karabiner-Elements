#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_environment.hpp"

namespace krbn::manipulator::conditions {
struct condition_context final {
  device_id device_id;
  event_queue::state state;
};

class base {
protected:
  base() {
  }

public:
  virtual ~base() {
  }

  virtual bool is_fulfilled(const condition_context& condition_context,
                            const manipulator_environment& manipulator_environment) const = 0;
};
} // namespace krbn::manipulator::conditions
