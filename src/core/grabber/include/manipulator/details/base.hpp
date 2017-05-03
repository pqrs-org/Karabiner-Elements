#pragma once

#include "event_queue.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class base {
protected:
  base(void) : valid_(true) {
  }

public:
  virtual ~base(void) {
  }

  virtual void manipulate(event_queue& event_queue, uint64_t time_stamp) = 0;

  virtual bool active(void) const = 0;

  bool get_valid(void) const {
    return valid_;
  }
  void set_valid(bool value) {
    valid_ = value;
  }

private:
  bool valid_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
