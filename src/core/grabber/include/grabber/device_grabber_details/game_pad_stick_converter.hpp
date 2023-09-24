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
  static constexpr int xy_timer_interval = 20;
  static constexpr int wheel_timer_interval = 100;

  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const event_queue::entry&)> pointing_motion_arrived;

  //
  // Methods
  //

  game_pad_stick_converter(void)
      : dispatcher_client(),
        xy_timer_(*this),
        xy_timer_active_(false),
        wheel_timer_(*this),
        wheel_timer_active_(false) {
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      wheel_timer_.stop();
      xy_timer_.stop();
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
                // -1.0 ... 1.0
                auto value = ((static_cast<double>(v.get_integer_value() - *logical_min) /
                               (*logical_max - *logical_min)) -
                              0.5) *
                             2;

                // TODO: Move config
                double deadzone = 0.05;
                if (value > 0.0) {
                  if (value < deadzone) {
                    value = 0.0;
                  } else {
                    value = (value - deadzone) / (1.0 - deadzone);
                  }
                } else if (value < 0.0) {
                  if (value > -deadzone) {
                    value = 0.0;
                  } else {
                    value = (value + deadzone) / (1.0 - deadzone);
                  }
                }

                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  auto integer_value = static_cast<int>(value * 127);
                  integer_value = std::min(integer_value, 127);
                  integer_value = std::max(integer_value, -127);

                  it->second.set_x(integer_value);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  auto integer_value = static_cast<int>(value * 127);
                  integer_value = std::min(integer_value, 127);
                  integer_value = std::max(integer_value, -127);

                  it->second.set_y(integer_value);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  int integer_value = 0;
                  if (std::abs(value) > 0.5) {
                    integer_value = 2 * (std::signbit(value) ? -1 : 1);
                  } else if (std::abs(value) > 0) {
                    integer_value = 1 * (std::signbit(value) ? -1 : 1);
                  }

                  it->second.set_vertical_wheel(-1 * integer_value);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  int integer_value = 0;
                  if (std::abs(value) > 0.5) {
                    integer_value = 2 * (std::signbit(value) ? -1 : 1);
                  } else if (std::abs(value) > 0) {
                    integer_value = 1 * (std::signbit(value) ? -1 : 1);
                  }

                  it->second.set_horizontal_wheel(integer_value);
                }
              }
            }
          }
        }
      }
    }

    logger::get_logger()->info("pointing_motion: {0},{1},{2},{3}",
                               it->second.get_x(),
                               it->second.get_y(),
                               it->second.get_vertical_wheel(),
                               it->second.get_horizontal_wheel());

    set_timer();
  }

private:
  void set_timer(void) {
    //
    // xy
    //

    if (!xy_timer_active_) {
      xy_timer_.start(
          [this] {
            bool needs_stop = true;

            for (const auto& [device_id, device_pointing_motion] : states_) {
              pointing_motion m(device_pointing_motion.get_x(),
                                device_pointing_motion.get_y(),
                                0,
                                0);

              if (!m.is_zero()) {
                event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
                event_queue::event event(m);
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
              xy_timer_.stop();
              xy_timer_active_ = false;
            }
          },
          std::chrono::milliseconds(xy_timer_interval));

      xy_timer_active_ = true;
    }

    //
    // wheel
    //

    if (!wheel_timer_active_) {
      wheel_timer_.start(
          [this] {
            bool needs_stop = true;

            for (const auto& [device_id, device_pointing_motion] : states_) {
              pointing_motion m(0,
                                0,
                                device_pointing_motion.get_vertical_wheel(),
                                device_pointing_motion.get_horizontal_wheel());

              if (!m.is_zero()) {
                event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
                event_queue::event event(m);
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
              wheel_timer_.stop();
              wheel_timer_active_ = false;
            }
          },
          std::chrono::milliseconds(wheel_timer_interval));

      wheel_timer_active_ = true;
    }
  }

  pqrs::dispatcher::extra::timer xy_timer_;
  bool xy_timer_active_;
  pqrs::dispatcher::extra::timer wheel_timer_;
  bool wheel_timer_active_;
  std::unordered_map<device_id, pointing_motion> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
