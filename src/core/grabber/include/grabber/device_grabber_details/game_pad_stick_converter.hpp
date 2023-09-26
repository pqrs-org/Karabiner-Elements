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
  struct state final {
    state(void)
        : x(0),
          x_interval(0),
          x_active(false),
          y(0),
          y_interval(0),
          y_active(false),
          vertical_wheel(0),
          vertical_wheel_interval(0),
          vertical_wheel_active(false),
          horizontal_wheel(0),
          horizontal_wheel_interval(0),
          horizontal_wheel_active(false) {
    }

    int x;
    std::chrono::milliseconds x_interval;
    bool x_active;

    int y;
    std::chrono::milliseconds y_interval;
    bool y_active;

    int vertical_wheel;
    std::chrono::milliseconds vertical_wheel_interval;
    bool vertical_wheel_active;

    int horizontal_wheel;
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
      : dispatcher_client() {
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher();
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
                auto double_value = ((static_cast<double>(v.get_integer_value() - *logical_min) / (*logical_max - *logical_min)) - 0.5) * 2.0;

                // TODO: Move config
                const double deadzone = 0.05;

                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  it->second.x = calculate_xy_value(double_value,
                                                    deadzone);
                  it->second.x_interval = calculate_xy_interval(double_value,
                                                                deadzone);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  it->second.y = calculate_xy_value(double_value,
                                                    deadzone);
                  it->second.y_interval = calculate_xy_interval(double_value,
                                                                deadzone);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  it->second.vertical_wheel = -1 * calculate_wheel_value(double_value,
                                                                         deadzone);
                  it->second.vertical_wheel_interval = calculate_wheel_interval(double_value,
                                                                                deadzone);

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  it->second.horizontal_wheel = calculate_wheel_value(double_value,
                                                                      deadzone);
                  it->second.horizontal_wheel_interval = calculate_wheel_interval(double_value,
                                                                                  deadzone);
                }
              }
            }
          }
        }
      }
    }

    // logger::get_logger()->info("pointing_motion: {0},{1},{2},{3}",
    //                            it->second.x,
    //                            it->second.y,
    //                            it->second.vertical_wheel,
    //                            it->second.horizontal_wheel);

    for (auto&& [device_id, state] : states_) {
      if (!state.x_active &&
          state.x_interval > std::chrono::milliseconds(0)) {
        state.x_active = true;
        emit_x_event(device_id);
      }

      if (!state.y_active &&
          state.y_interval > std::chrono::milliseconds(0)) {
        state.y_active = true;
        emit_y_event(device_id);
      }

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

private:
  int calculate_xy_value(double double_value,
                         double deadzone) const {
    int mode = 0; // 0: linear, 1: quadratic
    if (mode == 0) {
      // Linear
      if (double_value > 0.0) {
        if (double_value < deadzone) {
          double_value = 0.0;
        } else {
          double_value = (double_value - deadzone) / (1.0 - deadzone);
        }
      } else if (double_value < 0.0) {
        if (double_value > -deadzone) {
          double_value = 0.0;
        } else {
          double_value = (double_value + deadzone) / (1.0 - deadzone);
        }
      }
    } else {
      // Quadratic
      if (double_value > 0.0) {
        if (double_value < deadzone) {
          double_value = 0.0;
        } else {
          double_value = ((double_value - deadzone) * (double_value - deadzone)) / ((1.0 - deadzone) * (1.0 - deadzone));
        }
      } else if (double_value < 0.0) {
        if (double_value > -deadzone) {
          double_value = 0.0;
        } else {
          double_value = ((double_value + deadzone) * (double_value + deadzone)) / ((1.0 - deadzone) * (1.0 - deadzone));
        }
      }
    }

    // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
    auto result = static_cast<int>(double_value * 32);
    result = std::min(result, 32);
    result = std::max(result, -32);
    return result;
  }

  std::chrono::milliseconds calculate_xy_interval(double double_value,
                                                  double deadzone) const {
    if (std::abs(double_value) > 0.8) {
      return std::chrono::milliseconds(5);
    } else if (std::abs(double_value) > 0.5) {
      return std::chrono::milliseconds(10);
    } else if (std::abs(double_value) > deadzone) {
      return std::chrono::milliseconds(20);
    }

    return std::chrono::milliseconds(0);
  }

  int calculate_wheel_value(double double_value,
                            double deadzone) const {
    // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
    return std::signbit(double_value) ? -1 : 1;
  }

  std::chrono::milliseconds calculate_wheel_interval(double double_value,
                                                     double deadzone) const {
    if (std::abs(double_value) > 0.8) {
      return std::chrono::milliseconds(25);
    } else if (std::abs(double_value) > 0.5) {
      return std::chrono::milliseconds(50);
    } else if (std::abs(double_value) > deadzone) {
      return std::chrono::milliseconds(100);
    }

    return std::chrono::milliseconds(0);
  }

  void emit_event(const device_id device_id,
                  std::function<std::chrono::milliseconds(const state&)> get_interval,
                  std::function<void(state&)> unset_active,
                  std::function<pointing_motion(const state&)> make_pointing_motion) {
    auto it = states_.find(device_id);
    if (it == std::end(states_)) {
      return;
    }

    auto interval = get_interval(it->second);

    if (interval <= std::chrono::milliseconds(0)) {
      unset_active(it->second);
      return;
    }

    auto m = make_pointing_motion(it->second);

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
        [this, device_id, get_interval, unset_active, make_pointing_motion, entry] {
          pointing_motion_arrived(entry);

          emit_event(device_id,
                     get_interval,
                     unset_active,
                     make_pointing_motion);
        },
        when_now() + interval);
  }

  void emit_x_event(const device_id device_id) {
    auto get_interval = [](const state& s) {
      return s.x_interval;
    };

    auto unset_active = [](state& s) {
      s.x_active = false;
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(s.x,
                             0,
                             0,
                             0);
    };

    emit_event(device_id,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_y_event(const device_id device_id) {
    auto get_interval = [](const state& s) {
      return s.y_interval;
    };

    auto unset_active = [](state& s) {
      s.y_active = false;
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             s.y,
                             0,
                             0);
    };

    emit_event(device_id,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_vertical_wheel_event(const device_id device_id) {
    auto get_interval = [](const state& s) {
      return s.vertical_wheel_interval;
    };

    auto unset_active = [](state& s) {
      s.vertical_wheel_active = false;
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             0,
                             s.vertical_wheel,
                             0);
    };

    emit_event(device_id,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_horizontal_wheel_event(const device_id device_id) {
    auto get_interval = [](const state& s) {
      return s.horizontal_wheel_interval;
    };

    auto unset_active = [](state& s) {
      s.horizontal_wheel_active = false;
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             0,
                             0,
                             s.horizontal_wheel);
    };

    emit_event(device_id,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  std::unordered_map<device_id, state> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
