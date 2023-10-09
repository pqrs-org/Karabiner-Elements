#pragma once

#include "exprtk_utility.hpp"
#include "logger.hpp"
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
  class history final {
  public:
    history(absolute_time_point time_stamp,
            double radian,
            double magnitude)
        : time_stamp_(time_stamp),
          radian_(radian),
          magnitude_(magnitude) {
    }

    absolute_time_point get_time_stamp(void) const {
      return time_stamp_;
    }

    double get_radian(void) const {
      return radian_;
    }

    double get_magnitude(void) const {
      return magnitude_;
    }

  private:
    absolute_time_point time_stamp_;
    double radian_;
    double magnitude_;
  };

  class stick_sensor {
  public:
    double get_value(void) const {
      return value_;
    }

    void update_stick_sensor_value(CFIndex logical_max,
                                   CFIndex logical_min,
                                   CFIndex integer_value) {
      if (logical_max != logical_min) {
        // -1.0 ... 1.0
        value_ = ((static_cast<double>(integer_value - logical_min) / static_cast<double>(logical_max - logical_min)) - 0.5) * 2.0;
      }
    }

  private:
    double value_; // -1.0 ... 1.0
  };

  class stick final {
  public:
    stick(void)
        : active_(false) {
    }

    bool get_active(void) const {
      return active_;
    }

    void set_active(bool value) {
      active_ = value;
    }

    void update_horizontal_stick_sensor_value(CFIndex logical_max,
                                              CFIndex logical_min,
                                              CFIndex integer_value,
                                              double deadzone) {
      horizontal_stick_sensor_.update_stick_sensor_value(logical_max,
                                                         logical_min,
                                                         integer_value);
      update(deadzone);
    }

    void update_vertical_stick_sensor_value(CFIndex logical_max,
                                            CFIndex logical_min,
                                            CFIndex integer_value,
                                            double deadzone) {
      vertical_stick_sensor_.update_stick_sensor_value(logical_max,
                                                       logical_min,
                                                       integer_value);
      update(deadzone);
    }

    void update_xy_formula(const std::string& x_formula_string,
                           const std::string& y_formula_string) {
      x_formula_string_ = x_formula_string;
      y_formula_string_ = y_formula_string;

      x_formula_ = make_formula_expression(x_formula_string_);
      y_formula_ = make_formula_expression(y_formula_string_);
    }

    std::pair<int, int> xy_value(void) const {
      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      auto x = x_formula_.value();
      if (std::isnan(x)) {
        logger::get_logger()->error("game_pad_stick_converter x_formula returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    x_formula_string_,
                                    radian_,
                                    magnitude_,
                                    holding_acceleration_);
        x = 0.0;
      }

      auto y = y_formula_.value();
      if (std::isnan(y)) {
        logger::get_logger()->error("game_pad_stick_converter y_formula returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    y_formula_string_,
                                    radian_,
                                    magnitude_,
                                    holding_acceleration_);
        y = 0.0;
      }

      return std::make_pair(adjust_integer_value(static_cast<int>(x * 127)),
                            adjust_integer_value(static_cast<int>(y * 127)));
    }

    std::pair<int, int> wheels_value(void) const {
      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      auto h = std::cos(radian_);
      auto v = std::sin(radian_);

      return std::make_pair(std::signbit(v) ? -1 : 1,
                            std::signbit(h) ? -1 : 1);
    }

    std::chrono::milliseconds xy_interval(void) const {
      if (holding_acceleration_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      return std::chrono::milliseconds(20);
    }

    std::chrono::milliseconds wheels_interval(void) const {
      if (holding_acceleration_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      if (holding_acceleration_ > 0.3) {
        return std::chrono::milliseconds(50);
      }

      if (holding_acceleration_ > 0.1) {
        return std::chrono::milliseconds(75);
      }

      return std::chrono::milliseconds(100);
    }

  private:
    void update(double deadzone) {
      radian_ = std::atan2(vertical_stick_sensor_.get_value(),

                           horizontal_stick_sensor_.get_value());
      magnitude_ = std::min(1.0,
                            std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                      std::pow(horizontal_stick_sensor_.get_value(), 2)));

      //
      // Update holding_acceleration_, holding_magnitude_
      //

      if (std::abs(vertical_stick_sensor_.get_value()) < deadzone &&
          std::abs(horizontal_stick_sensor_.get_value()) < deadzone) {
        histories_.clear();
        holding_acceleration_ = 0.0;
        holding_magnitude_ = 0.0;
        return;
      }

      auto now = pqrs::osx::chrono::mach_absolute_time_point();

      histories_.erase(std::remove_if(std::begin(histories_),
                                      std::end(histories_),
                                      [now](const auto& h) {
                                        auto interval = pqrs::osx::chrono::make_milliseconds(now - h.get_time_stamp()).count();
                                        return interval > 20;
                                      }),
                       histories_.end());

      histories_.push_back(history(now,
                                   radian_,
                                   magnitude_));

      auto [min, max] = std::minmax_element(std::begin(histories_),
                                            std::end(histories_),
                                            [](const auto& a, const auto& b) {
                                              return a.get_magnitude() < b.get_magnitude();
                                            });
      if (min != std::end(histories_) && max != std::end(histories_)) {
        auto acceleration = max->get_magnitude() - min->get_magnitude();

        if (holding_acceleration_ < acceleration) {
          // Increase acceleration if magnitude is increased.
          if (magnitude_ > holding_magnitude_) {
            holding_acceleration_ = acceleration;
          }
        } else {
          // Decrease acceleration if magnitude is decreased.
          if (magnitude_ < holding_magnitude_ - 0.1) {
            holding_acceleration_ = acceleration;
          }
        }

        holding_magnitude_ = magnitude_;
      }
    }

    exprtk_utility::expression_t make_formula_expression(const std::string& formula) {
      return exprtk_utility::compile(formula,
                                     {},
                                     {
                                         {"radian", radian_},
                                         {"magnitude", magnitude_},
                                         {"acceleration", holding_acceleration_},
                                     });
    }

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;
    double radian_;
    double magnitude_;
    double holding_acceleration_;
    double holding_magnitude_;
    std::vector<history> histories_;

    std::string x_formula_string_;
    std::string y_formula_string_;
    exprtk_utility::expression_t x_formula_;
    exprtk_utility::expression_t y_formula_;

    bool active_; // Whether the process of calling `pointing_motion_arrived` is working or not
  };

  class state final {
  public:
    state(void) {
    }

    state(const device_identifiers& di)
        : device_identifiers_(di) {
    }

    void update_formula(const core_configuration::core_configuration& core_configuration) {
      xy.update_xy_formula(core_configuration.get_selected_profile().get_device_game_pad_stick_x_formula(device_identifiers_),
                           core_configuration.get_selected_profile().get_device_game_pad_stick_y_formula(device_identifiers_));
    }

    stick xy;
    stick wheels;

  private:
    device_identifiers device_identifiers_;
  };

  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const event_queue::entry&)> pointing_motion_arrived;

  //
  // Methods
  //

  game_pad_stick_converter(std::weak_ptr<const core_configuration::core_configuration> core_configuration)
      : dispatcher_client() {
    set_core_configuration(core_configuration);
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher();
  }

  void set_core_configuration(std::weak_ptr<const core_configuration::core_configuration> core_configuration) {
    core_configuration_ = core_configuration;

    if (auto c = core_configuration_.lock()) {
      for (auto&& [device_id, state] : states_) {
        state.update_formula(*c);
      }
    }
  }

  void register_device(const device_properties& device_properties) {
    if (auto is_game_pad = device_properties.get_is_game_pad()) {
      if (*is_game_pad) {
        auto s = state(device_properties.get_device_identifiers());
        if (auto c = core_configuration_.lock()) {
          s.update_formula(*c);
        }

        states_[device_properties.get_device_id()] = s;
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

    auto c = core_configuration_.lock();
    if (!c) {
      return;
    }

    auto left_stick_deadzone = c->get_selected_profile().get_device_game_pad_stick_left_stick_deadzone(device_properties.get_device_identifiers());
    auto right_stick_deadzone = c->get_selected_profile().get_device_game_pad_stick_right_stick_deadzone(device_properties.get_device_identifiers());

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
                    it->second.wheels.update_horizontal_stick_sensor_value(*logical_max,
                                                                           *logical_min,
                                                                           v.get_integer_value(),
                                                                           right_stick_deadzone);
                  } else {
                    it->second.xy.update_horizontal_stick_sensor_value(*logical_max,
                                                                       *logical_min,
                                                                       v.get_integer_value(),
                                                                       left_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (flip_sticks) {
                    it->second.wheels.update_vertical_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value(),
                                                                         right_stick_deadzone);
                  } else {
                    it->second.xy.update_vertical_stick_sensor_value(*logical_max,
                                                                     *logical_min,
                                                                     v.get_integer_value(),
                                                                     left_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (flip_sticks) {
                    it->second.xy.update_vertical_stick_sensor_value(*logical_max,
                                                                     *logical_min,
                                                                     v.get_integer_value(),
                                                                     left_stick_deadzone);
                  } else {
                    it->second.wheels.update_vertical_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value(),
                                                                         right_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (flip_sticks) {
                    it->second.xy.update_horizontal_stick_sensor_value(*logical_max,
                                                                       *logical_min,
                                                                       v.get_integer_value(),
                                                                       left_stick_deadzone);
                  } else {
                    it->second.wheels.update_horizontal_stick_sensor_value(*logical_max,
                                                                           *logical_min,
                                                                           v.get_integer_value(),
                                                                           right_stick_deadzone);
                  }
                }
              }
            }
          }
        }
      }
    }

    for (auto&& [device_id, state] : states_) {
      if (!state.xy.get_active() &&
          state.xy.xy_interval().count() > 0) {
        state.xy.set_active(true);
        emit_xy_event(device_id,
                      event_origin);
      }

      if (!state.wheels.get_active() &&
          state.wheels.wheels_interval().count() > 0) {
        state.wheels.set_active(true);
        emit_wheels_event(device_id,
                          event_origin);
      }
    }
  }

  static int adjust_integer_value(int value) {
    // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
    value = std::min(value, 127);
    value = std::max(value, -127);
    return value;
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

    pointing_motion_arrived(entry);

    enqueue_to_dispatcher(
        [this, device_id, event_origin, get_interval, unset_active, make_pointing_motion, entry] {
          emit_event(device_id,
                     event_origin,
                     get_interval,
                     unset_active,
                     make_pointing_motion);
        },
        when_now() + interval);
  }

  void emit_xy_event(const device_id device_id,
                     event_origin event_origin) {
    auto get_interval = [](const state& s) {
      return s.xy.xy_interval();
    };

    auto unset_active = [](state& s) {
      s.xy.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      auto [x, y] = s.xy.xy_value();

      return pointing_motion(x,
                             y,
                             0,
                             0);
    };

    emit_event(device_id,
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  void emit_wheels_event(const device_id device_id,
                         event_origin event_origin) {
    auto get_interval = [](const state& s) {
      return s.wheels.wheels_interval();
    };

    auto unset_active = [](state& s) {
      s.wheels.set_active(false);
    };

    auto make_pointing_motion = [](const state& s) {
      auto [v, h] = s.wheels.wheels_value();

      return pointing_motion(0,
                             0,
                             -v,
                             h);
    };

    emit_event(device_id,
               event_origin,
               get_interval,
               unset_active,
               make_pointing_motion);
  }

  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  std::unordered_map<device_id, state> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
