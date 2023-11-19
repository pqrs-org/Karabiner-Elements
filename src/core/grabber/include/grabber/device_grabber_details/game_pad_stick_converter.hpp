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
  //
  // Signals (invoked from the dispatcher thread)
  //

  typedef nod::signal<void(const event_queue::entry&)> pointing_motion_arrived_t;

  pointing_motion_arrived_t pointing_motion_arrived;

  //
  // Classes
  //

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
    static constexpr int update_timer_interval_milliseconds = 20;

    stick(void)
        : dispatcher_client(),
          deadzone_timer_(*this),
          update_timer_(*this),
          radian_(0.0),
          magnitude_(0.0),
          stroke_acceleration_destination_value_(0.0),
          stroke_acceleration_transition_value_(0.0),
          stroke_acceleration_transition_magnitude_(0.0),
          deadzone_magnitude_(0.0),
          previous_horizontal_value_(0.0),
          previous_vertical_value_(0.0),
          stick_stroke_release_detection_threshold_milliseconds_(0),
          stick_stroke_acceleration_measurement_duration_milliseconds_(0) {
      auto now = pqrs::osx::chrono::mach_absolute_time_point();
      deadzone_entered_at_ = now;
      deadzone_left_at_ = now;
    }

    ~stick(void) {
      detach_from_dispatcher([this] {
        deadzone_timer_.stop();
        update_timer_.stop();
      });
    }

    void update_horizontal_stick_sensor_value(CFIndex logical_max,
                                              CFIndex logical_min,
                                              CFIndex integer_value,
                                              double deadzone) {
      horizontal_stick_sensor_.update_stick_sensor_value(logical_max,
                                                         logical_min,
                                                         integer_value);
      set_update_timer(deadzone);
    }

    void update_vertical_stick_sensor_value(CFIndex logical_max,
                                            CFIndex logical_min,
                                            CFIndex integer_value,
                                            double deadzone) {
      vertical_stick_sensor_.update_stick_sensor_value(logical_max,
                                                       logical_min,
                                                       integer_value);
      set_update_timer(deadzone);
    }

    void set_update_timer(double deadzone) {
      // The deadzone may be changed even while update_timer is active.
      // Therefore, we always update regardless of the update_timer status.
      enqueue_to_dispatcher([this, deadzone] {
        deadzone_ = deadzone;
      });

      if (!update_timer_.enabled()) {
        update_timer_.start(
            [this] {
              update_values();
            },
            std::chrono::milliseconds(update_timer_interval_milliseconds));
      }
    }

    void update_configurations(const core_configuration::core_configuration& core_configuration,
                               const device_identifiers& device_identifiers) {
      stick_stroke_release_detection_threshold_milliseconds_ = core_configuration.get_selected_profile().get_device_game_pad_stick_stroke_release_detection_threshold_milliseconds(device_identifiers);
      stick_stroke_acceleration_measurement_duration_milliseconds_ = core_configuration.get_selected_profile().get_device_game_pad_stick_stroke_acceleration_measurement_duration_milliseconds_(device_identifiers);
      stick_stroke_acceleration_transition_duration_milliseconds_ = core_configuration.get_selected_profile().get_device_game_pad_stick_stroke_acceleration_transition_duration_milliseconds(device_identifiers);

      xy_stick_interval_milliseconds_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_xy_stick_interval_milliseconds_formula(device_identifiers);
      wheels_stick_interval_milliseconds_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_wheels_stick_interval_milliseconds_formula(device_identifiers);
      x_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_x_formula(device_identifiers);
      y_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_y_formula(device_identifiers);
      vertical_wheel_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_vertical_wheel_formula(device_identifiers);
      horizontal_wheel_formula_string_ = core_configuration.get_selected_profile().get_device_game_pad_stick_horizontal_wheel_formula(device_identifiers);

      xy_stick_interval_milliseconds_formula_ = make_formula_expression(xy_stick_interval_milliseconds_formula_string_);
      wheels_stick_interval_milliseconds_formula_ = make_formula_expression(wheels_stick_interval_milliseconds_formula_string_);
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
                                    stroke_acceleration_transition_value_);
        x = 0.0;
      }

      auto y = y_formula_.value();
      if (std::isnan(y)) {
        logger::get_logger()->error("game_pad_stick_converter y_formula returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    y_formula_string_,
                                    radian_,
                                    magnitude_,
                                    stroke_acceleration_transition_value_);
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

    std::chrono::milliseconds xy_stick_interval(void) const {
      if (stroke_acceleration_transition_value_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      auto interval = xy_stick_interval_milliseconds_formula_.value();
      if (std::isnan(interval)) {
        logger::get_logger()->error("game_pad_stick_converter xy_stick_interval_milliseconds_formula_ returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    xy_stick_interval_milliseconds_formula_string_,
                                    radian_,
                                    magnitude_,
                                    stroke_acceleration_transition_value_);
        interval = 0;
      }

      return std::chrono::milliseconds(static_cast<int>(interval));
    }

    std::chrono::milliseconds wheels_stick_interval(void) const {
      if (stroke_acceleration_transition_value_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      auto interval = wheels_stick_interval_milliseconds_formula_.value();
      if (std::isnan(interval)) {
        logger::get_logger()->error("game_pad_stick_converter wheels_stick_interval_milliseconds_formula_ returns nan: {0} (radian: {1}, magnitude: {2}, acceleration: {3})",
                                    wheels_stick_interval_milliseconds_formula_string_,
                                    radian_,
                                    magnitude_,
                                    stroke_acceleration_transition_value_);
        interval = 0;
      }

      return std::chrono::milliseconds(static_cast<int>(interval));
    }

  private:
    void update_values(void) {
      auto now = pqrs::osx::chrono::mach_absolute_time_point();

      auto delta_vertical = vertical_stick_sensor_.get_value() - previous_vertical_value_;
      auto delta_horizontal = horizontal_stick_sensor_.get_value() - previous_horizontal_value_;
      auto delta_radian = std::atan2(delta_vertical, delta_horizontal);
      auto delta_magnitude = std::min(1.0,
                                      std::sqrt(
                                          std::pow(delta_horizontal, 2) +
                                          std::pow(delta_vertical, 2)));

      radian_ = std::atan2(vertical_stick_sensor_.get_value(),
                           horizontal_stick_sensor_.get_value());

      magnitude_ = std::min(1.0,
                            std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                      std::pow(horizontal_stick_sensor_.get_value(), 2)));

      //
      // Update stroke_acceleration_
      //

      if (std::abs(vertical_stick_sensor_.get_value()) < deadzone_ &&
          std::abs(horizontal_stick_sensor_.get_value()) < deadzone_) {
        deadzone_magnitude_ = magnitude_;
        delta_magnitude = 0.0;

        if (!deadzone_timer_.enabled()) {
          deadzone_timer_.start(
              [this, now] {
                deadzone_entered_at_ = now;
                stroke_acceleration_destination_value_ = 0.0;
                stroke_acceleration_transition_value_ = 0.0;
                stroke_acceleration_transition_magnitude_ = 0.0;

                update_timer_.stop();
              },
              std::chrono::milliseconds(stick_stroke_release_detection_threshold_milliseconds_));
        }
      } else {
        deadzone_timer_.stop();

        if (deadzone_left_at_ < deadzone_entered_at_) {
          deadzone_left_at_ = now;
        }
      }

      if (delta_magnitude > 0.0) {
        auto radian_diff = std::fmod(std::abs(delta_radian - radian_), 2 * M_PI);
        if (radian_diff > M_PI) {
          radian_diff = 2 * M_PI - radian_diff;
        }

        const auto threshold = 0.174533; // 10 degree
        auto stroke_acceleration_destination_value_changed = false;
        if (radian_diff < threshold) {
          stroke_acceleration_destination_value_ += delta_magnitude;
          if (stroke_acceleration_destination_value_ > 1.0) {
            stroke_acceleration_destination_value_ = 1.0;
          }

          stroke_acceleration_destination_value_changed = true;

        } else if (radian_diff > M_PI - threshold && radian_diff < M_PI + threshold) {
          stroke_acceleration_destination_value_ -= delta_magnitude;
          if (stroke_acceleration_destination_value_ < 0.0) {
            stroke_acceleration_destination_value_ = 0.0;
          }

          stroke_acceleration_destination_value_changed = true;
        }

        if (stroke_acceleration_destination_value_changed) {
          auto divisor = std::max(1, stick_stroke_acceleration_transition_duration_milliseconds_ / update_timer_interval_milliseconds);
          stroke_acceleration_transition_magnitude_ = std::abs(stroke_acceleration_destination_value_ - stroke_acceleration_transition_value_) / divisor;
        }
      }

      if (stroke_acceleration_transition_value_ != stroke_acceleration_destination_value_) {
        if (stroke_acceleration_transition_value_ < stroke_acceleration_destination_value_) {
          stroke_acceleration_transition_value_ += stroke_acceleration_transition_magnitude_;
        } else if (stroke_acceleration_transition_value_ > stroke_acceleration_destination_value_) {
          stroke_acceleration_transition_value_ -= stroke_acceleration_transition_magnitude_;
        }

        if (std::abs(stroke_acceleration_transition_value_ - stroke_acceleration_destination_value_) < stroke_acceleration_transition_magnitude_) {
          stroke_acceleration_transition_value_ = stroke_acceleration_destination_value_;
        }
      }

      previous_vertical_value_ = vertical_stick_sensor_.get_value();
      previous_horizontal_value_ = horizontal_stick_sensor_.get_value();
    }

    exprtk_utility::expression_t make_formula_expression(const std::string& formula) {
      return exprtk_utility::compile(formula,
                                     {},
                                     {
                                         {"radian", radian_},
                                         {"magnitude", magnitude_},
                                         {"acceleration", stroke_acceleration_transition_value_},
                                     });
    }

    int adjust_integer_value(int value) const {
      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      value = std::min(value, 127);
      value = std::max(value, -127);
      return value;
    }

    pqrs::dispatcher::extra::timer deadzone_timer_;
    pqrs::dispatcher::extra::timer update_timer_;

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;

    double deadzone_;
    double radian_;
    double magnitude_;
    double stroke_acceleration_destination_value_;
    double stroke_acceleration_transition_value_;
    double stroke_acceleration_transition_magnitude_;
    absolute_time_point deadzone_entered_at_;
    absolute_time_point deadzone_left_at_;
    double deadzone_magnitude_;
    double previous_horizontal_value_;
    double previous_vertical_value_;

    //
    // configurations
    //

    int stick_stroke_release_detection_threshold_milliseconds_;
    int stick_stroke_acceleration_measurement_duration_milliseconds_;
    int stick_stroke_acceleration_transition_duration_milliseconds_;
    std::string xy_stick_interval_milliseconds_formula_string_;
    std::string wheels_stick_interval_milliseconds_formula_string_;
    std::string x_formula_string_;
    std::string y_formula_string_;
    std::string vertical_wheel_formula_string_;
    std::string horizontal_wheel_formula_string_;
    exprtk_utility::expression_t xy_stick_interval_milliseconds_formula_;
    exprtk_utility::expression_t wheels_stick_interval_milliseconds_formula_;
    exprtk_utility::expression_t x_formula_;
    exprtk_utility::expression_t y_formula_;
    exprtk_utility::expression_t vertical_wheel_formula_;
    exprtk_utility::expression_t horizontal_wheel_formula_;
  };

  class state final : public pqrs::dispatcher::extra::dispatcher_client {
  public:
    state(device_id device_id,
          const device_identifiers& di,
          const pointing_motion_arrived_t& pointing_motion_arrived)
        : dispatcher_client(),
          device_id_(device_id),
          device_identifiers_(di),
          pointing_motion_arrived_(pointing_motion_arrived),
          xy_deadzone_(0.0),
          wheels_deadzone_(0.0),
          xy_timer_(*this),
          wheels_timer_(*this) {
    }

    ~state(void) {
      detach_from_dispatcher([this] {
        xy_timer_.stop();
        wheels_timer_.stop();
      });
    }

    void update_configurations(const core_configuration::core_configuration& core_configuration) {
      xy_.update_configurations(core_configuration,
                                device_identifiers_);
      wheels_.update_configurations(core_configuration,
                                    device_identifiers_);

      xy_deadzone_ = core_configuration.get_selected_profile().get_device_game_pad_xy_stick_deadzone(device_identifiers_);
      wheels_deadzone_ = core_configuration.get_selected_profile().get_device_game_pad_wheels_stick_deadzone(device_identifiers_);
    }

    void update_x_stick_sensor_value(CFIndex logical_max,
                                     CFIndex logical_min,
                                     CFIndex integer_value) {
      xy_.update_horizontal_stick_sensor_value(logical_max,
                                               logical_min,
                                               integer_value,
                                               xy_deadzone_);
    }

    void update_y_stick_sensor_value(CFIndex logical_max,
                                     CFIndex logical_min,
                                     CFIndex integer_value) {
      xy_.update_vertical_stick_sensor_value(logical_max,
                                             logical_min,
                                             integer_value,
                                             xy_deadzone_);
    }

    void update_vertical_wheel_stick_sensor_value(CFIndex logical_max,
                                                  CFIndex logical_min,
                                                  CFIndex integer_value) {
      wheels_.update_vertical_stick_sensor_value(logical_max,
                                                 logical_min,
                                                 integer_value,
                                                 wheels_deadzone_);
    }

    void update_horizontal_wheel_stick_sensor_value(CFIndex logical_max,
                                                    CFIndex logical_min,
                                                    CFIndex integer_value) {
      wheels_.update_horizontal_stick_sensor_value(logical_max,
                                                   logical_min,
                                                   integer_value,
                                                   wheels_deadzone_);
    }

    void update_xy_timer(event_origin event_origin) {
      auto interval = xy_.xy_stick_interval();
      if (interval == std::chrono::milliseconds(0)) {
        xy_timer_.stop();
        return;
      }

      if (xy_timer_.enabled()) {
        return;
      }

      xy_timer_.start(
          [this, event_origin] {
            //
            // Update interval
            //

            auto interval = xy_.xy_stick_interval();
            if (interval == std::chrono::milliseconds(0)) {
              xy_timer_.stop();
              return;
            }
            xy_timer_.set_interval(interval);

            //
            // Post event
            //

            auto [x, y] = xy_.xy_value();

            pointing_motion m(x,
                              y,
                              0,
                              0);

            event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
            event_queue::event event(m);
            event_queue::entry entry(device_id_,
                                     event_time_stamp,
                                     event,
                                     event_type::single,
                                     event,
                                     event_origin,
                                     event_queue::state::original);

            enqueue_to_dispatcher([this, entry] {
              pointing_motion_arrived_(entry);
            });
          },
          interval);
    }

    void update_wheels_timer(event_origin event_origin) {
      auto interval = wheels_.wheels_stick_interval();
      if (interval == std::chrono::milliseconds(0)) {
        wheels_timer_.stop();
        return;
      }

      if (wheels_timer_.enabled()) {
        return;
      }

      wheels_timer_.start(
          [this, event_origin] {
            //
            // Update interval
            //

            auto interval = wheels_.wheels_stick_interval();
            if (interval == std::chrono::milliseconds(0)) {
              wheels_timer_.stop();
              return;
            }
            wheels_timer_.set_interval(interval);

            //
            // Post event
            //

            auto [v, h] = wheels_.wheels_value();

            pointing_motion m(0,
                              0,
                              -v,
                              h);

            event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
            event_queue::event event(m);
            event_queue::entry entry(device_id_,
                                     event_time_stamp,
                                     event,
                                     event_type::single,
                                     event,
                                     event_origin,
                                     event_queue::state::original);

            enqueue_to_dispatcher([this, entry] {
              pointing_motion_arrived_(entry);
            });
          },
          interval);
    }

  private:
    device_id device_id_;
    device_identifiers device_identifiers_;
    const pointing_motion_arrived_t& pointing_motion_arrived_;

    double xy_deadzone_;
    double wheels_deadzone_;

    stick xy_;
    stick wheels_;

    pqrs::dispatcher::extra::timer xy_timer_;
    pqrs::dispatcher::extra::timer wheels_timer_;
  };

  //
  // Methods
  //

  game_pad_stick_converter(std::weak_ptr<const core_configuration::core_configuration> core_configuration)
      : dispatcher_client() {
    set_core_configuration(core_configuration);
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      states_.clear();
    });
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
        auto s = std::make_shared<state>(device_properties.get_device_id(),
                                         device_properties.get_device_identifiers(),
                                         pointing_motion_arrived);
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

    bool swap_sticks = c->get_selected_profile().get_device_game_pad_swap_sticks(device_properties.get_device_identifiers());

    for (const auto& v : hid_values) {
      if (auto usage_page = v.get_usage_page()) {
        if (auto usage = v.get_usage()) {
          if (auto logical_max = v.get_logical_max()) {
            if (auto logical_min = v.get_logical_min()) {
              if (*logical_max != *logical_min) {
                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  if (swap_sticks) {
                    it->second->update_horizontal_wheel_stick_sensor_value(*logical_max,
                                                                           *logical_min,
                                                                           v.get_integer_value());
                  } else {
                    it->second->update_x_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (swap_sticks) {
                    it->second->update_vertical_wheel_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value());
                  } else {
                    it->second->update_y_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (swap_sticks) {
                    it->second->update_y_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value());
                  } else {
                    it->second->update_vertical_wheel_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (swap_sticks) {
                    it->second->update_x_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value());
                  } else {
                    it->second->update_horizontal_wheel_stick_sensor_value(*logical_max,
                                                                           *logical_min,
                                                                           v.get_integer_value());
                  }
                }
              }
            }
          }
        }
      }
    }

    it->second->update_xy_timer(event_origin);
    it->second->update_wheels_timer(event_origin);
  }

private:
  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  std::unordered_map<device_id, std::shared_ptr<state>> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
