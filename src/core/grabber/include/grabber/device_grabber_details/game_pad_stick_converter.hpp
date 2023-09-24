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
  static constexpr int timer_interval = 20;

  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const event_queue::entry&)> pointing_motion_arrived;

  //
  // Methods
  //

  game_pad_stick_converter(void)
      : dispatcher_client(),
        timer_(*this) {
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void register_device(const device_properties& device_properties) {
    if (auto is_game_pad = device_properties.get_is_game_pad()) {
      if (*is_game_pad) {
        states_[device_properties.get_device_id()] = pointing_motion();
      }
    }
  }

  void unregister_device(device_id device_id) {
    states_.erase(device_id);
  }

  void convert(const device_properties& device_properties,
               const std::vector<pqrs::osx::iokit_hid_value>& hid_values) {
    auto it = states_.find(device_properties.get_device_id());
    if (it == std::end(states_)) {
      return;
    }

    for (const auto& v : hid_values) {
      if (auto usage_page = v.get_usage_page()) {
        if (auto usage = v.get_usage()) {
          if (auto logical_max = v.get_logical_max()) {
            if (auto logical_min = v.get_logical_min()) {
              if (*logical_max != *logical_min) {
                // 0.0 ... 1.0
                auto value = static_cast<double>(v.get_integer_value() - *logical_min) /
                             (*logical_max - *logical_min);
                // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                auto integer_value = static_cast<int>((value - 0.5) * 127);
                integer_value = std::min(integer_value, 127);
                integer_value = std::max(integer_value, -127);

                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  it->second.set_x(-1 * integer_value);
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  it->second.set_y(-1 * integer_value);
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  it->second.set_vertical_wheel(-1 * integer_value);
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  it->second.set_horizontal_wheel(-1 * integer_value);
                }
              }
            }
          }
        }
      }
    }

    set_timer();
  }

private:
  void set_timer(void) {
    timer_.start(
        [this] {
          bool needs_stop = true;

          for (const auto& [device_id, pointing_motion] : states_) {
            if (!pointing_motion.is_zero()) {
              event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
              event_queue::event event(pointing_motion);
              event_queue::entry entry(device_id,
                                       event_time_stamp,
                                       event,
                                       event_type::single,
                                       event,
                                       event_origin::grabbed_device,
                                       event_queue::state::original);

              enqueue_to_dispatcher([this, entry] {
                pointing_motion_arrived(entry);
              });

              needs_stop = false;
            }
          }

          if (needs_stop) {
            timer_.stop();
          }
        },
        std::chrono::milliseconds(timer_interval));
  }

  pqrs::dispatcher::extra::timer timer_;
  std::unordered_map<device_id, pointing_motion> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
