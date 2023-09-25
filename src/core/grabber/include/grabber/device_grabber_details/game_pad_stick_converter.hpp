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

  struct state final {
    state(void)
        : x(0),
          y(0),
          vertical_wheel(0),
          horizontal_wheel(0),
          x_interval(0),
          y_interval(0),
          vertical_wheel_interval(0),
          horizontal_wheel_interval(0) {
    }

    int x;
    int y;
    int vertical_wheel;
    int horizontal_wheel;
    std::chrono::milliseconds x_interval;
    std::chrono::milliseconds y_interval;
    std::chrono::milliseconds vertical_wheel_interval;
    bool vertical_wheel_active;
    std::chrono::milliseconds horizontal_wheel_interval;
    bool horizontal_wheel_active;
  };

  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const event_queue::entry&)>
      pointing_motion_arrived;

  //
  // Methods
  //

  game_pad_stick_converter(void)
      : dispatcher_client(),
        xy_timer_(*this),
        xy_timer_active_(false) {
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      xy_timer_.stop();
    });
  }

  void register_device(const device_properties& device_properties) {
    if (auto is_game_pad = device_properties.get_is_game_pad()) {
      if (*is_game_pad) {
        states_[device_properties.get_device_id()] = state();
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
                const double deadzone = 0.04;
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

                  it->second.x = integer_value;

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  auto integer_value = static_cast<int>(value * 127);
                  integer_value = std::min(integer_value, 127);
                  integer_value = std::max(integer_value, -127);

                  it->second.y = integer_value;

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  it->second.vertical_wheel = std::signbit(value) ? 1 : -1;

                  std::chrono::milliseconds interval(0);
                  if (std::abs(value) > 0.8) {
                    interval = std::chrono::milliseconds(25);
                  } else if (std::abs(value) > 0.5) {
                    interval = std::chrono::milliseconds(50);
                  } else if (std::abs(value) > deadzone) {
                    interval = std::chrono::milliseconds(100);
                  }
                  it->second.vertical_wheel_interval = interval;

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                  it->second.horizontal_wheel = std::signbit(value) ? -1 : 1;

                  std::chrono::milliseconds interval(0);
                  if (std::abs(value) > 0.8) {
                    interval = std::chrono::milliseconds(25);
                  } else if (std::abs(value) > 0.5) {
                    interval = std::chrono::milliseconds(50);
                  } else if (std::abs(value) > deadzone) {
                    interval = std::chrono::milliseconds(100);
                  }
                  it->second.horizontal_wheel_interval = interval;
                }
              }
            }
          }
        }
      }
    }

    logger::get_logger()->info("pointing_motion: {0},{1},{2},{3}",
                               it->second.x,
                               it->second.y,
                               it->second.vertical_wheel,
                               it->second.horizontal_wheel);

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

            for (const auto& [device_id, state] : states_) {
              pointing_motion m(state.x,
                                state.y,
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

    for (auto&& [device_id, state] : states_) {
      if (!state.vertical_wheel_active &&
          state.vertical_wheel_interval > std::chrono::milliseconds(0)) {
        state.vertical_wheel_active = true;
        emit_vertical_wheel_event(device_id);
      }

      if (!state.horizontal_wheel_active &&
          state.horizontal_wheel_interval > std::chrono::milliseconds(0)) {
        state.horizontal_wheel_active = true;
        emit_horizontal_wheel_event(device_id);
      }
    }
  }

  void emit_vertical_wheel_event(const device_id device_id) {
    auto it = states_.find(device_id);
    if (it == std::end(states_)) {
      return;
    }

    if (it->second.vertical_wheel_interval == std::chrono::milliseconds(0)) {
      it->second.vertical_wheel_active = false;
      return;
    }

    pointing_motion m(0,
                      0,
                      it->second.vertical_wheel,
                      0);

    event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
    event_queue::event event(m);
    event_queue::entry entry(device_id,
                             event_time_stamp,
                             event,
                             event_type::single,
                             event,
                             event_origin::grabbed_device,
                             event_queue::state::original);

    enqueue_to_dispatcher(
        [this, device_id, entry] {
          pointing_motion_arrived(entry);

          emit_vertical_wheel_event(device_id);
        },
        when_now() + it->second.vertical_wheel_interval);
  }

  void emit_horizontal_wheel_event(const device_id device_id) {
    auto it = states_.find(device_id);
    if (it == std::end(states_)) {
      return;
    }

    if (it->second.horizontal_wheel_interval <= std::chrono::milliseconds(0)) {
      it->second.horizontal_wheel_active = false;
      return;
    }

    pointing_motion m(0,
                      0,
                      0,
                      it->second.horizontal_wheel);

    event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
    event_queue::event event(m);
    event_queue::entry entry(device_id,
                             event_time_stamp,
                             event,
                             event_type::single,
                             event,
                             event_origin::grabbed_device,
                             event_queue::state::original);

    enqueue_to_dispatcher(
        [this, device_id, entry] {
          pointing_motion_arrived(entry);

          emit_horizontal_wheel_event(device_id);
        },
        when_now() + it->second.horizontal_wheel_interval);
  }

  pqrs::dispatcher::extra::timer xy_timer_;
  bool xy_timer_active_;
  std::unordered_map<device_id, state> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
