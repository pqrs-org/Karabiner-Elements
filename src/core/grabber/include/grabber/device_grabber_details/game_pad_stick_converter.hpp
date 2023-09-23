#pragma once

#include "types/device_id.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace krbn {
namespace grabber {
namespace device_grabber_details {
//
// game_pad_stick_converter takes value_arrived data as input and outputs poinitng motion.
// This conversion is necessary because it is difficult to use game pad sticks as natural pointing devices due to the following characteristics.
//
// - The game pad's stick only sends events when the value changes.
//   We want the pointer to move while the stick is tilted, even if the value does not change.
//   So we need to send events periodically with a timer.
// - The game pad's stick may send events slightly in the opposite direction when it is released and returns to neutral.
//   This event should be properly ignored.
// - The game pad's stick may have a value in the neutral state. The deadzone must be properly set and ignored neutral values.
//
class game_pad_stick_converter final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  game_pad_stick_converter(void)
      : dispatcher_client(),
        timer_(*this) {
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void convert(std::shared_ptr<event_queue::queue> event_queue) {
  }

private:
  pqrs::dispatcher::extra::timer timer_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
