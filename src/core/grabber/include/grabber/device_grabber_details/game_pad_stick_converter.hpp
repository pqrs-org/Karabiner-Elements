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

  class stick final : public pqrs::dispatcher::extra::dispatcher_client {
  public:
    stick(void)
        : dispatcher_client(),
          deadzone_timer_(*this),
          remain_deadzone_threshold_milliseconds_(0),
          stroke_acceleration_measurement_milliseconds_(0),
          xy_interval_milliseconds_(0),
          wheels_interval_milliseconds_(0),
          active_(false) {
      auto now = pqrs::osx::chrono::mach_absolute_time_point();
      deadzone_entered_at_ = now;
      deadzone_left_at_ = now;
    }

    ~stick(void) {
      detach_from_dispatcher([this] {
        deadzone_timer_.stop();
      });
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

    void update_configurations(const core_configuration::core_configuration& core_configuration,
                               const device_identifiers& device_identifiers) {
      // TODO: Add config
      remain_deadzone_threshold_milliseconds_ = 100;
      stroke_acceleration_measurement_milliseconds_ = 50;

      xy_interval_milliseconds_ = core_configuration.get_selected_profile().get_device_game_pad_stick_xy_interval_milliseconds(device_identifiers);
      wheels_interval_milliseconds_ = core_configuration.get_selected_profile().get_device_game_pad_stick_wheels_interval_milliseconds(device_identifiers);

      x_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_x_formula(device_identifiers);
      y_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_y_formula(device_identifiers);
      vertical_wheel_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_vertical_wheel_formula(device_identifiers);
      horizontal_wheel_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_horizontal_wheel_formula(device_identifiers);

      x_formula_ = make_formula_expression(x_formula_string_);
      y_formula_ = make_formula_expression(y_formula_string_);
      vertical_wheel_formula_ = make_formula_expression(vertical_wheel_formula_string_);
      horizontal_wheel_formula_ = make_formula_expression(horizontal_wheel_formula_string_);
    }

    std::pair<int, int> xy_value(void) const {
      auto x = x_formula_.value();
      if (std::isnan(x)) {
        logger::get_logger()->error("game_pad_stick_converter x_formula returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    x_formula_string_,
                                    radian_,
                                    magnitude_,
                                    stroke_acceleration_);
        x = 0.0;
      }

      auto y = y_formula_.value();
      if (std::isnan(y)) {
        logger::get_logger()->error("game_pad_stick_converter y_formula returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    y_formula_string_,
                                    radian_,
                                    magnitude_,
                                    stroke_acceleration_);
        y = 0.0;
      }

      return std::make_pair(adjust_integer_value(static_cast<int>(x)),
                            adjust_integer_value(static_cast<int>(y)));
    }

    std::pair<int, int> wheels_value(void) const {
      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      auto v = vertical_wheel_formula_.value();
      auto h = horizontal_wheel_formula_.value();

      return std::make_pair(adjust_integer_value(static_cast<int>(v)),
                            adjust_integer_value(static_cast<int>(h)));
    }

    std::chrono::milliseconds xy_interval(void) const {
      if (stroke_acceleration_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      return std::chrono::milliseconds(xy_interval_milliseconds_);
    }

    std::chrono::milliseconds wheels_interval(void) const {
      if (stroke_acceleration_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      return std::chrono::milliseconds(wheels_interval_milliseconds_);
    }

  private:
    void update(double deadzone) {
      auto now = pqrs::osx::chrono::mach_absolute_time_point();

      radian_ = std::atan2(vertical_stick_sensor_.get_value(),

                           horizontal_stick_sensor_.get_value());
      magnitude_ = std::min(1.0,
                            std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                      std::pow(horizontal_stick_sensor_.get_value(), 2)));

      //
      // Update stroke_acceleration_
      //

      if (std::abs(vertical_stick_sensor_.get_value()) < deadzone &&
          std::abs(horizontal_stick_sensor_.get_value()) < deadzone) {
        if (!deadzone_timer_.enabled()) {
          deadzone_timer_.start(
              [this, now] {
                deadzone_entered_at_ = now;
                stroke_acceleration_ = 0.0;
              },
              std::chrono::milliseconds(remain_deadzone_threshold_milliseconds_));
        }
      } else {
        deadzone_timer_.stop();

        if (deadzone_left_at_ < deadzone_entered_at_) {
          deadzone_left_at_ = now;
        }
      }

      if (pqrs::osx::chrono::make_milliseconds(now - deadzone_left_at_).count() < stroke_acceleration_measurement_milliseconds_) {
        stroke_acceleration_ = std::max(stroke_acceleration_, magnitude_);
      }
    }

    exprtk_utility::expression_t make_formula_expression(const std::string& formula) {
      return exprtk_utility::compile(formula,
                                     {},
                                     {
                                         {"radian", radian_},
                                         {"magnitude", magnitude_},
                                         {"acceleration", stroke_acceleration_},
                                     });
    }

    pqrs::dispatcher::extra::timer deadzone_timer_;

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;

    double radian_;
    double magnitude_;
    absolute_time_point deadzone_entered_at_;
    absolute_time_point deadzone_left_at_;
    double stroke_acceleration_;

    int remain_deadzone_threshold_milliseconds_;
    int stroke_acceleration_measurement_milliseconds_;
    int xy_interval_milliseconds_;
    int wheels_interval_milliseconds_;
    std::string x_formula_string_;
    std::string y_formula_string_;
    std::string vertical_wheel_formula_string_;
    std::string horizontal_wheel_formula_string_;
    exprtk_utility::expression_t x_formula_;
    exprtk_utility::expression_t y_formula_;
    exprtk_utility::expression_t vertical_wheel_formula_;
    exprtk_utility::expression_t horizontal_wheel_formula_;

    bool active_; // Whether the process of calling `pointing_motion_arrived` is working or not
  };

  class state final {
  public:
    state(void) {
    }

    state(const device_identifiers& di)
        : device_identifiers_(di) {
    }

    void update_configurations(const core_configuration::core_configuration& core_configuration) {
      xy.update_configurations(core_configuration,
                               device_identifiers_);
      wheels.update_configurations(core_configuration,
                                   device_identifiers_);
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
        state->update_configurations(*c);
      }
    }
  }

  void register_device(const device_properties& device_properties) {
    if (auto is_game_pad = device_properties.get_is_game_pad()) {
      if (*is_game_pad) {
        auto s = std::make_shared<state>(device_properties.get_device_identifiers());
        if (auto c = core_configuration_.lock()) {
          s->update_configurations(*c);
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
                    it->second->wheels.update_horizontal_stick_sensor_value(*logical_max,
                                                                            *logical_min,
                                                                            v.get_integer_value(),
                                                                            right_stick_deadzone);
                  } else {
                    it->second->xy.update_horizontal_stick_sensor_value(*logical_max,
                                                                        *logical_min,
                                                                        v.get_integer_value(),
                                                                        left_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (flip_sticks) {
                    it->second->wheels.update_vertical_stick_sensor_value(*logical_max,
                                                                          *logical_min,
                                                                          v.get_integer_value(),
                                                                          right_stick_deadzone);
                  } else {
                    it->second->xy.update_vertical_stick_sensor_value(*logical_max,
                                                                      *logical_min,
                                                                      v.get_integer_value(),
                                                                      left_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (flip_sticks) {
                    it->second->xy.update_vertical_stick_sensor_value(*logical_max,
                                                                      *logical_min,
                                                                      v.get_integer_value(),
                                                                      left_stick_deadzone);
                  } else {
                    it->second->wheels.update_vertical_stick_sensor_value(*logical_max,
                                                                          *logical_min,
                                                                          v.get_integer_value(),
                                                                          right_stick_deadzone);
                  }

                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (flip_sticks) {
                    it->second->xy.update_horizontal_stick_sensor_value(*logical_max,
                                                                        *logical_min,
                                                                        v.get_integer_value(),
                                                                        left_stick_deadzone);
                  } else {
                    it->second->wheels.update_horizontal_stick_sensor_value(*logical_max,
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
      if (!state->xy.get_active() &&
          state->xy.xy_interval().count() > 0) {
        state->xy.set_active(true);
        emit_xy_event(device_id,
                      event_origin);
      }

      if (!state->wheels.get_active() &&
          state->wheels.wheels_interval().count() > 0) {
        state->wheels.set_active(true);
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

    auto interval = get_interval(*(it->second));

    if (interval <= std::chrono::milliseconds(0)) {
      unset_active(*(it->second));
      return;
    }

    auto m = make_pointing_motion(*(it->second));

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
  std::unordered_map<device_id, std::shared_ptr<state>> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
