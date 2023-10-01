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
  class stick_event final {
  public:
    stick_event(absolute_time_point time_stamp,
                double double_value,
                double acceleration)
        : time_stamp_(time_stamp),
          double_value_(double_value),
          acceleration_(acceleration) {
    }

    absolute_time_point get_time_stamp(void) const {
      return time_stamp_;
    }

    double get_double_value(void) const {
      return double_value_;
    }

    double get_acceleration(void) const {
      return acceleration_;
    }

    double attenuated_acceleration(absolute_time_point now) const {
      auto interval = static_cast<double>(pqrs::osx::chrono::make_milliseconds(now - time_stamp_).count()) / 1000.0;
      auto attenuation = 5.0 * interval;
      if (std::abs(acceleration_) < attenuation) {
        return 0.0;
      }

      if (acceleration_ > 0) {
        return acceleration_ - attenuation;
      } else {
        return acceleration_ + attenuation;
      }
    }

  private:
    absolute_time_point time_stamp_;
    double double_value_; // -1.0 ... 1.0
    double acceleration_;
  };

  class stick final {
  public:
    stick(void)
        : stick_value_(0.0),
          active_(false) {
    }

    double get_stick_value(void) const {
      return stick_value_;
    }

    bool get_active(void) const {
      return active_;
    }

    void set_active(bool value) {
      active_ = value;
    }

    int integer_value(void) const {
      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      auto divider = 50.0;
      auto result = static_cast<int>(stick_value_ / divider);
      result = std::min(result, 127);
      result = std::max(result, -127);
      return result;
    }

    std::chrono::milliseconds interval(void) const {
      if (std::abs(stick_value_) == 0.0) {
        return std::chrono::milliseconds(0);
      }

      return std::chrono::milliseconds(20);
    }

    void add_event(CFIndex logical_max,
                   CFIndex logical_min,
                   CFIndex integer_value) {
      if (logical_max != logical_min) {
        // -1.0 ... 1.0
        auto double_value = ((static_cast<double>(integer_value - logical_min) / static_cast<double>(logical_max - logical_min)) - 0.5) * 2.0;
        auto now = pqrs::osx::chrono::mach_absolute_time_point();

        double previous_double_value = 0.0;
        auto previous_time_stamp = now;
        if (stick_events_.size() > 0) {
          previous_double_value = stick_events_.back().get_double_value();
          previous_time_stamp = stick_events_.back().get_time_stamp();
        }

        auto interval_seconds = std::max(
            static_cast<double>(pqrs::osx::chrono::make_milliseconds(now - previous_time_stamp).count()) / 1000.0,
            0.001 // 1 ms
        );

        // -1000.0 ... 1000.0
        auto acceleration = (double_value - previous_double_value) / interval_seconds;

        stick_events_.push_back(stick_event(now,
                                            double_value,
                                            acceleration));
      }
    }

    void update(void) {
      if (stick_events_.empty()) {
        stick_value_ = 0.0;
        return;
      }

      double deadzone = 0.1;
      if (std::abs(stick_events_.back().get_double_value()) < deadzone) {
        stick_events_.clear();
        stick_value_ = 0.0;

      } else {
        auto now = pqrs::osx::chrono::mach_absolute_time_point();

        double sum = 0.0;
        for (const auto& stick_event : stick_events_) {
          sum += stick_event.attenuated_acceleration(now);
        }
        stick_value_ = sum;

        stick_events_.erase(std::remove_if(stick_events_.begin(),
                                           stick_events_.end(),
                                           [now, sum](const auto& e) {
                                             auto attenuated_acceleration = e.attenuated_acceleration(now);
                                             return attenuated_acceleration == 0.0 || sum * attenuated_acceleration < 0;
                                           }),
                            stick_events_.end());
      }
    }

  private:
    double stick_value_; // Close to the additive value of acceleration
    bool active_;        // Whether the process of calling `pointing_motion_arrived` is working or not
    std::vector<stick_event> stick_events_;
  };

  struct state final {
    stick x;
    stick y;
    stick vertical_wheel;
    stick horizontal_wheel;
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
                // TODO: Move config
                const bool flip_sticks = false;

                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  if (flip_sticks) {
                    it->second.horizontal_wheel.add_event(*logical_max,
                                                          *logical_min,
                                                          v.get_integer_value());

                    it->second.vertical_wheel.update();
                    it->second.horizontal_wheel.update();
                  } else {
                    it->second.x.add_event(*logical_max,
                                           *logical_min,
                                           v.get_integer_value());

                    it->second.x.update();
                    it->second.y.update();
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (flip_sticks) {
                    it->second.vertical_wheel.add_event(*logical_max,
                                                        *logical_min,
                                                        v.get_integer_value());

                    it->second.vertical_wheel.update();
                    it->second.horizontal_wheel.update();
                  } else {
                    it->second.y.add_event(*logical_max,
                                           *logical_min,
                                           v.get_integer_value());

                    it->second.x.update();
                    it->second.y.update();
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (flip_sticks) {
                    it->second.y.add_event(*logical_max,
                                           *logical_min,
                                           v.get_integer_value());

                    it->second.x.update();
                    it->second.y.update();
                  } else {
                    it->second.vertical_wheel.add_event(*logical_max,
                                                        *logical_min,
                                                        v.get_integer_value());

                    it->second.vertical_wheel.update();
                    it->second.horizontal_wheel.update();
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (flip_sticks) {
                    it->second.x.add_event(*logical_max,
                                           *logical_min,
                                           v.get_integer_value());

                    it->second.x.update();
                    it->second.y.update();
                  } else {
                    it->second.horizontal_wheel.add_event(*logical_max,
                                                          *logical_min,
                                                          v.get_integer_value());

                    it->second.vertical_wheel.update();
                    it->second.horizontal_wheel.update();
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
      if (!state.x.get_active() &&
          std::abs(state.x.get_stick_value()) > 0.0) {
        state.x.set_active(true);
        emit_x_event(device_id,
                     event_origin);
      }

      if (!state.y.get_active() &&
          std::abs(state.y.get_stick_value()) > 0.0) {
        state.y.set_active(true);
        emit_y_event(device_id,
                     event_origin);
      }

      if (!state.vertical_wheel.get_active() &&
          std::abs(state.vertical_wheel.get_stick_value()) > 0.0) {
        state.vertical_wheel.set_active(true);
        emit_vertical_wheel_event(device_id,
                                  event_origin);
      }

      if (!state.horizontal_wheel.get_active() &&
          std::abs(state.horizontal_wheel.get_stick_value()) > 0.0) {
        state.horizontal_wheel.set_active(true);
        emit_horizontal_wheel_event(device_id,
                                    event_origin);
      }
    }
  }

private:
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
      return s.x.interval();
    };

    auto unset_active = [](state& s) {
      s.x.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(s.x.integer_value(),
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
      return s.y.interval();
    };

    auto unset_active = [](state& s) {
      s.y.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             s.y.integer_value(),
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
      return s.vertical_wheel.interval();
    };

    auto unset_active = [](state& s) {
      s.vertical_wheel.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             0,
                             -s.vertical_wheel.integer_value(),
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
      return s.horizontal_wheel.interval();
    };

    auto unset_active = [](state& s) {
      s.horizontal_wheel.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      return pointing_motion(0,
                             0,
                             0,
                             s.horizontal_wheel.integer_value());
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
