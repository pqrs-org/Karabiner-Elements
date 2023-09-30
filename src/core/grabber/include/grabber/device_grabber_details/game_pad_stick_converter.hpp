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
    struct value final {
    public:
      value(void)
          : integer_value(0),
            double_value(0.0),
            interval(0),
            active(false) {
      }

      void set_double_value(double value) {
        double_value = value;

        auto now = pqrs::osx::chrono::mach_absolute_time_point();
        histories.push_back(std::make_pair(now, value));

        auto it = std::remove_if(histories.begin(),
                                 histories.end(),
                                 [now](const auto& h) {
                                   auto duration = pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(300));
                                   return now - h.first > duration;
                                 });
        histories.erase(it, std::end(histories));
      }

      double acceleration(void) {
        if (histories.size() == 0) {
          return 0.0;
        }

        const auto [min, max] = std::minmax_element(histories.begin(),
                                                    histories.end(),
                                                    [](const auto& a, const auto& b) {
                                                      return a.second < b.second;
                                                    });
        if (max == histories.end()) {
          return 0.0;
        }
        if (min == histories.end()) {
          return 0.0;
        }

        auto duration = std::abs(make_milliseconds(max->first - min->first).count());
        if (duration == 0) {
          return 0.0;
        }

        return (max->second - min->second) / duration;
      }

      int integer_value;   // Value to be sent to the virtual devices
      double double_value; // -1.0 ... 1.0
      std::chrono::milliseconds interval;
      bool active; // Whether the process of calling `pointing_motion_arrived` is working or not
      std::vector<std::pair<absolute_time_point, double>> histories;
    };

    state(void)
        : y(0),
          y_double_value(0.0),
          y_interval(0),
          y_active(false),
          vertical_wheel(0),
          vertical_wheel_double_value(0.0),
          vertical_wheel_interval(0),
          vertical_wheel_active(false),
          horizontal_wheel(0),
          horizontal_wheel_double_value(0.0),
          horizontal_wheel_interval(0),
          horizontal_wheel_active(false) {
    }

    value x;

    int y;
    double y_double_value; // -1.0 ... 1.0
    std::chrono::milliseconds y_interval;
    bool y_active;
    std::vector<std::pair<absolute_time_point, double>> y_histories;

    int vertical_wheel;
    double vertical_wheel_double_value; // -1.0 ... 1.0
    std::chrono::milliseconds vertical_wheel_interval;
    bool vertical_wheel_active;

    int horizontal_wheel;
    double horizontal_wheel_double_value; // -1.0 ... 1.0
    std::chrono::milliseconds horizontal_wheel_interval;
    bool horizontal_wheel_active;
  };

  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const event_queue::entry&)> pointing_motion_arrived;

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
               const std::vector<pqrs::osx::iokit_hid_value>& hid_values,
               event_origin event_origin) {
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
                const bool flip_sticks = false;

                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  if (flip_sticks) {
                    it->second.horizontal_wheel = calculate_wheel_value(double_value,
                                                                        deadzone);
                    it->second.horizontal_wheel_interval = calculate_wheel_interval(double_value,
                                                                                    deadzone);
                  } else {
                    it->second.x.set_double_value(double_value);
                    logger::get_logger()->info("acceleration: {0}", it->second.x.acceleration());
                    update_xy(it->second, deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (flip_sticks) {
                    it->second.vertical_wheel = -1 * calculate_wheel_value(double_value,
                                                                           deadzone);
                    it->second.vertical_wheel_interval = calculate_wheel_interval(double_value,
                                                                                  deadzone);
                  } else {
                    it->second.y_double_value = double_value;
                    update_xy(it->second, deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (flip_sticks) {
                    it->second.y = calculate_xy_value(double_value,
                                                      deadzone);
                    it->second.y_interval = calculate_xy_interval(double_value,
                                                                  deadzone);
                  } else {
                    it->second.vertical_wheel_double_value = double_value;
                    it->second.vertical_wheel = -1 * calculate_wheel_value(double_value,
                                                                           deadzone);
                    it->second.vertical_wheel_interval = calculate_wheel_interval(double_value,
                                                                                  deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (flip_sticks) {
                    it->second.x.double_value = double_value;
                    update_xy(it->second, deadzone);
                  } else {
                    it->second.horizontal_wheel_double_value = double_value;
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
    }

    // logger::get_logger()->info("pointing_motion: {0},{1},{2},{3}",
    //                            it->second.x,
    //                            it->second.y,
    //                            it->second.vertical_wheel,
    //                            it->second.horizontal_wheel);

    for (auto&& [device_id, state] : states_) {
      if (!state.x.active &&
          state.x.interval > std::chrono::milliseconds(0)) {
        state.x.active = true;
        emit_x_event(device_id,
                     event_origin);
      }

      if (!state.y_active &&
          state.y_interval > std::chrono::milliseconds(0)) {
        state.y_active = true;
        emit_y_event(device_id,
                     event_origin);
      }

      if (!state.vertical_wheel_active &&
          state.vertical_wheel_interval > std::chrono::milliseconds(0)) {
        state.vertical_wheel_active = true;
        emit_vertical_wheel_event(device_id,
                                  event_origin);
      }

      if (!state.horizontal_wheel_active &&
          state.horizontal_wheel_interval > std::chrono::milliseconds(0)) {
        state.horizontal_wheel_active = true;
        emit_horizontal_wheel_event(device_id,
                                    event_origin);
      }
    }
  }

private:
  void update_xy(state& s, double deadzone) const {
    double magnitude = std::sqrt((s.x.double_value * s.x.double_value) + (s.y_double_value * s.y_double_value));
    logger::get_logger()->info("magnitude: {0}: {1},{2}", magnitude, s.x.double_value, s.y_double_value);

    int scale = 0;
    if (magnitude > 0.9) {
      scale = 4;
    } else if (magnitude > 0.5) {
      scale = 3;
    } else if (magnitude > 0.25) {
      scale = 2;
    } else {
      scale = 1;
    }

    if (std::abs(s.x.double_value) > deadzone) {
      s.x.integer_value = scale * (std::signbit(s.x.double_value) ? -1 : 1);
      s.x.interval = std::chrono::milliseconds(20);
    } else {
      s.x.integer_value = 0.0;
      s.x.interval = std::chrono::milliseconds(0);
    }

    if (std::abs(s.y_double_value) > deadzone) {
      s.y = scale * (std::signbit(s.y_double_value) ? -1 : 1);
      s.y_interval = std::chrono::milliseconds(20);
    } else {
      s.y = 0.0;
      s.y_interval = std::chrono::milliseconds(0);
    }
  }

  int calculate_xy_value(double double_value,
                         double deadzone) const {
    int mode = 2; // 0: linear, 1: quadratic, 2: custom
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
    } else if (mode == 1) {
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
          double_value = -1 * ((double_value + deadzone) * (double_value + deadzone)) / ((1.0 - deadzone) * (1.0 - deadzone));
        }
      }
    } else if (mode == 2) {
      if (std::abs(double_value) > 0.9) {
        return 4 * (std::signbit(double_value) ? -1 : 1);
      } else if (std::abs(double_value) > 0.5) {
        return 2 * (std::signbit(double_value) ? -1 : 1);
      } else if (std::abs(double_value) > deadzone) {
        return 1 * (std::signbit(double_value) ? -1 : 1);
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
#if 0
    if (std::abs(double_value) > 0.8) {
      return std::chrono::milliseconds(5);
    } else if (std::abs(double_value) > 0.5) {
      return std::chrono::milliseconds(10);
    } else if (std::abs(double_value) > deadzone) {
      return std::chrono::milliseconds(20);
    }
#else
    if (std::abs(double_value) > 0.9) {
      return std::chrono::milliseconds(5);
    } else if (std::abs(double_value) > 0.75) {
      return std::chrono::milliseconds(10);
    } else if (std::abs(double_value) > 0.25) {
      return std::chrono::milliseconds(15);
    } else if (std::abs(double_value) > deadzone) {
      return std::chrono::milliseconds(20);
    }
#endif

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
                  event_origin event_origin,
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
                             event_origin,
                             event_queue::state::original);

    enqueue_to_dispatcher(
        [this, device_id, event_origin, get_interval, unset_active, make_pointing_motion, entry] {
          pointing_motion_arrived(entry);

          emit_event(device_id,
                     event_origin,
                     get_interval,
                     unset_active,
                     make_pointing_motion);
        },
        when_now() + interval);
  }

  void emit_x_event(const device_id device_id,
                    event_origin event_origin) {
    auto get_interval = [](const state& s) {
      return s.x.interval;
    };

    auto unset_active = [](state& s) {
      s.x.active = false;
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(s.x.integer_value,
                             0,
                             0,
                             0);
    };

    emit_event(device_id,
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_y_event(const device_id device_id,
                    event_origin event_origin) {
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
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_vertical_wheel_event(const device_id device_id,
                                 event_origin event_origin) {
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
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_horizontal_wheel_event(const device_id device_id,
                                   event_origin event_origin) {
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
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  std::unordered_map<device_id, state> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
