#pragma once

#include "exprtk_utility.hpp"
#include "logger.hpp"
#include "types/device_id.hpp"
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace krbn {
namespace grabber {
namespace device_grabber_details {
//
// game_pad_stick_converter takes value_arrived data as input and outputs poinitng motion.
// Due to the following characteristics, it is not possible to directly convert HID values from a gamepad into poinitng motion without using a converter.
//
// - The game pad's stick only sends events when the value changes.
//   We want the pointer to move while the stick is completely tilted, even if the value does not change.
//   So we need to send events periodically with a timer.
// - The game pad's stick may send events slightly in the opposite direction when it is released and returns to neutral.
//   This event should be properly ignored.
// - The game pad's stick may have slight values instead of (0,0) even in the neutral position.
//   It is also known that Hall Effect Joysticks repeat minute value changes in the neutral position.
//   We should ignore these values.
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

  enum class continued_movement_mode {
    none,
    xy,
    wheels,
  };

  class stick_sensor {
  public:
    stick_sensor(void)
        : value_(0) {
    }

    double get_value(void) const {
      return value_;
    }

    void update_stick_sensor_value(CFIndex logical_max,
                                   CFIndex logical_min,
                                   CFIndex integer_value) {
      if (logical_max != logical_min) {
        // -1.0 ... 1.0
        value_ = ((static_cast<double>(integer_value - logical_min) / static_cast<double>(logical_max - logical_min)) - 0.5) * 2.0;
      } else {
        value_ = 0;
      }
    }

  private:
    double value_; // -1.0 ... 1.0
  };

  class stick final : public pqrs::dispatcher::extra::dispatcher_client {
  public:
    //
    // Signals (invoked from the dispatcher thread)
    //

    nod::signal<void(void)> values_updated;

    //
    // Methods
    //

    stick(void)
        : dispatcher_client(),
          radian_(0.0),
          absolute_magnitude_(0.0),
          delta_magnitude_(0.0),
          previous_absolute_magnitude_(0.0),
          deadzone_(0.0),
          delta_magnitude_detection_threshold_(0.0),
          continued_movement_absolute_magnitude_threshold_(1.0),
          continued_movement_interval_milliseconds_(0) {
    }

    ~stick(void) {
      detach_from_dispatcher();
    }

    double get_radian(void) const {
      return radian_;
    }

    double get_absolute_magnitude(void) const {
      return absolute_magnitude_;
    }

    double get_delta_magnitude(void) const {
      return delta_magnitude_;
    }

    void set_deadzone(double value) {
      deadzone_ = value;
    }

    void set_delta_magnitude_detection_threshold(double value) {
      delta_magnitude_detection_threshold_ = value;
    }

    void set_continued_movement_absolute_magnitude_threshold(double value) {
      continued_movement_absolute_magnitude_threshold_ = value;
    }

    void set_continued_movement_interval_milliseconds(int value) {
      continued_movement_interval_milliseconds_ = value;
    }

    // This method should be called in the shared dispatcher thread.
    void update_horizontal_stick_sensor_value(CFIndex logical_max,
                                              CFIndex logical_min,
                                              CFIndex integer_value) {
      horizontal_stick_sensor_.update_stick_sensor_value(logical_max,
                                                         logical_min,
                                                         integer_value);

      update_values();
    }

    // This method should be called in the shared dispatcher thread.
    void update_vertical_stick_sensor_value(CFIndex logical_max,
                                            CFIndex logical_min,
                                            CFIndex integer_value) {
      vertical_stick_sensor_.update_stick_sensor_value(logical_max,
                                                       logical_min,
                                                       integer_value);

      update_values();
    }

    bool continued_movement(void) const {
      return absolute_magnitude_ >= continued_movement_absolute_magnitude_threshold_;
    }

    std::chrono::milliseconds get_continued_movement_interval_milliseconds(void) const {
      if (continued_movement()) {
        return std::chrono::milliseconds(continued_movement_interval_milliseconds_);
      }

      return std::chrono::milliseconds(0);
    }

  private:
    // This method is executed in the shared dispatcher thread.
    void update_values(void) {
      radian_ = std::atan2(vertical_stick_sensor_.get_value(),
                           horizontal_stick_sensor_.get_value());
      // When the stick is tilted diagonally, the distance from the centre may exceed 1.0 (e.g. 1.2).
      // Therefore, we correct the value <= 1.0.
      absolute_magnitude_ = std::min(1.0,
                                     std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                               std::pow(horizontal_stick_sensor_.get_value(), 2)));

      auto dm = std::max(0.0, absolute_magnitude_ - previous_absolute_magnitude_);

      //
      // Update delta_magnitude_
      //

      if (continued_movement()) {
        delta_magnitude_ = dm;
        previous_absolute_magnitude_ = absolute_magnitude_;
      } else {
        // Ignore minor magnitude changes until a sufficient amount of change accumulates.
        if (0 < dm && dm < delta_magnitude_detection_threshold_) {
          delta_magnitude_ = 0.0;
        } else {
          delta_magnitude_ = dm;
          previous_absolute_magnitude_ = absolute_magnitude_;
        }
      }

      if (absolute_magnitude_ <= deadzone_) {
        delta_magnitude_ = 0.0;
        absolute_magnitude_ = 0.0;
      }

      //
      // Signal
      //

      values_updated();
    }

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;

    double radian_;
    double absolute_magnitude_;
    double delta_magnitude_;
    double previous_absolute_magnitude_;

    //
    // configurations
    //

    double deadzone_;
    double delta_magnitude_detection_threshold_;
    double continued_movement_absolute_magnitude_threshold_;
    int continued_movement_interval_milliseconds_;
  };

  class event_value final {
  public:
    event_value(void)
        : value_(0.0),
          remainder_(0.0) {
    }

    void set_value(double value) {
      value_ = value;
      // Keep remainder_.
    }

    int truncated_value(void) {
      auto truncated = std::trunc(value_ + remainder_);
      remainder_ += value_ - truncated;

      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      auto v = static_cast<int>(truncated);
      v = std::min(v, 127);
      v = std::max(v, -127);
      return v;
    }

  private:
    double value_;
    double remainder_;
  };

  //
  // Methods
  //

  game_pad_stick_converter(gsl::not_null<std::shared_ptr<device_properties>> device_properties,
                           gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration)
      : dispatcher_client(),
        device_properties_(device_properties),
        core_configuration_(core_configuration),
        continued_movement_timer_(*this),
        continued_movement_timer_count_(0),
        continued_movement_mode_(continued_movement_mode::none),
        xy_radian_(0.0),
        xy_delta_magnitude_(0.0),
        xy_absolute_magnitude_(0.0),
        xy_continued_movement_(false),
        wheels_radian_(0.0),
        wheels_delta_magnitude_(0.0),
        wheels_absolute_magnitude_(0.0),
        wheels_continued_movement_(false) {
    set_core_configuration(core_configuration);

    xy_.values_updated.connect([this](void) {
      auto interval = xy_.get_continued_movement_interval_milliseconds();
      update_continued_movement_timer(continued_movement_mode::xy, interval);
    });

    wheels_.values_updated.connect([this](void) {
      auto interval = wheels_.get_continued_movement_interval_milliseconds();
      update_continued_movement_timer(continued_movement_mode::wheels, interval);
    });
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      continued_movement_timer_.stop();
    });
  }

  // This method should be called in the shared dispatcher thread.
  void update_x_stick_sensor_value(CFIndex logical_max,
                                   CFIndex logical_min,
                                   CFIndex integer_value) {
    xy_.update_horizontal_stick_sensor_value(logical_max,
                                             logical_min,
                                             integer_value);
  }

  // This method should be called in the shared dispatcher thread.
  void update_y_stick_sensor_value(CFIndex logical_max,
                                   CFIndex logical_min,
                                   CFIndex integer_value) {
    xy_.update_vertical_stick_sensor_value(logical_max,
                                           logical_min,
                                           integer_value);
  }

  // This method should be called in the shared dispatcher thread.
  void update_vertical_wheel_stick_sensor_value(CFIndex logical_max,
                                                CFIndex logical_min,
                                                CFIndex integer_value) {
    wheels_.update_vertical_stick_sensor_value(logical_max,
                                               logical_min,
                                               integer_value);
  }

  // This method should be called in the shared dispatcher thread.
  void update_horizontal_wheel_stick_sensor_value(CFIndex logical_max,
                                                  CFIndex logical_min,
                                                  CFIndex integer_value) {
    wheels_.update_horizontal_stick_sensor_value(logical_max,
                                                 logical_min,
                                                 integer_value);
  }

  void set_core_configuration(gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration) {
    core_configuration_ = core_configuration;

    //
    // Propagate the changes
    //

    auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());

    xy_.set_deadzone(d->get_game_pad_xy_stick_deadzone());
    xy_.set_delta_magnitude_detection_threshold(d->get_game_pad_xy_stick_delta_magnitude_detection_threshold());
    xy_.set_continued_movement_absolute_magnitude_threshold(d->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold());
    xy_.set_continued_movement_interval_milliseconds(d->get_game_pad_xy_stick_continued_movement_interval_milliseconds());

    wheels_.set_deadzone(d->get_game_pad_wheels_stick_deadzone());
    wheels_.set_delta_magnitude_detection_threshold(d->get_game_pad_wheels_stick_delta_magnitude_detection_threshold());
    wheels_.set_continued_movement_absolute_magnitude_threshold(d->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold());
    wheels_.set_continued_movement_interval_milliseconds(d->get_game_pad_wheels_stick_continued_movement_interval_milliseconds());

    x_formula_string_ = d->get_game_pad_stick_x_formula();
    y_formula_string_ = d->get_game_pad_stick_y_formula();
    vertical_wheel_formula_string_ = d->get_game_pad_stick_vertical_wheel_formula();
    horizontal_wheel_formula_string_ = d->get_game_pad_stick_horizontal_wheel_formula();

    x_formula_ = make_xy_formula_expression(x_formula_string_);
    y_formula_ = make_xy_formula_expression(y_formula_string_);
    vertical_wheel_formula_ = make_wheels_formula_expression(vertical_wheel_formula_string_);
    horizontal_wheel_formula_ = make_wheels_formula_expression(horizontal_wheel_formula_string_);
  }

  // This method should be called in the shared dispatcher thread.
  void convert(const std::vector<pqrs::osx::iokit_hid_value>& hid_values) {
    if (auto d = weak_dispatcher_.lock()) {
      if (!d->dispatcher_thread()) {
        logger::get_logger()->error("game_pad_stick_converter::convert is called in invalid thread");
        return;
      }
    }

    auto device = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    bool swap_sticks = device->get_game_pad_swap_sticks();

    for (const auto& v : hid_values) {
      if (auto usage_page = v.get_usage_page()) {
        if (auto usage = v.get_usage()) {
          if (auto logical_max = v.get_logical_max()) {
            if (auto logical_min = v.get_logical_min()) {
              if (*logical_max != *logical_min) {
                if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                  pqrs::hid::usage::generic_desktop::x)) {
                  if (swap_sticks) {
                    update_horizontal_wheel_stick_sensor_value(*logical_max,
                                                               *logical_min,
                                                               v.get_integer_value());
                  } else {
                    update_x_stick_sensor_value(*logical_max,
                                                *logical_min,
                                                v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (swap_sticks) {
                    update_vertical_wheel_stick_sensor_value(*logical_max,
                                                             *logical_min,
                                                             v.get_integer_value());
                  } else {
                    update_y_stick_sensor_value(*logical_max,
                                                *logical_min,
                                                v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (swap_sticks) {
                    update_y_stick_sensor_value(*logical_max,
                                                *logical_min,
                                                v.get_integer_value());
                  } else {
                    update_vertical_wheel_stick_sensor_value(*logical_max,
                                                             *logical_min,
                                                             v.get_integer_value());
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (swap_sticks) {
                    update_x_stick_sensor_value(*logical_max,
                                                *logical_min,
                                                v.get_integer_value());
                  } else {
                    update_horizontal_wheel_stick_sensor_value(*logical_max,
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
  }

private:
  exprtk_utility::expression_wrapper make_xy_formula_expression(const std::string& formula) {
    return exprtk_utility::compile(formula,
                                   {},
                                   {
                                       {"radian", xy_radian_},
                                       {"delta_magnitude", xy_delta_magnitude_},
                                       {"absolute_magnitude", xy_absolute_magnitude_},
                                       {"continued_movement", xy_continued_movement_},
                                   });
  }

  exprtk_utility::expression_wrapper make_wheels_formula_expression(const std::string& formula) {
    return exprtk_utility::compile(formula,
                                   {},
                                   {
                                       {"radian", wheels_radian_},
                                       {"delta_magnitude", wheels_delta_magnitude_},
                                       {"absolute_magnitude", wheels_absolute_magnitude_},
                                       {"continued_movement", wheels_continued_movement_},
                                   });
  }

  std::pair<double, double> xy_hid_values(void) const {
    auto x = x_formula_.value();
    if (std::isnan(x)) {
      logger::get_logger()->error("game_pad_stick_converter x_formula returns nan: {0}",
                                  x_formula_string_);
      x = 0.0;
    }

    auto y = y_formula_.value();
    if (std::isnan(y)) {
      logger::get_logger()->error("game_pad_stick_converter y_formula returns nan: {0}",
                                  y_formula_string_);
      y = 0.0;
    }

    return std::make_pair(x, y);
  }

  std::pair<double, double> wheels_hid_values(void) const {
    auto h = horizontal_wheel_formula_.value();
    if (std::isnan(h)) {
      logger::get_logger()->error("game_pad_stick_converter horizontal_wheel_formula returns nan: {0}",
                                  horizontal_wheel_formula_string_);
      h = 0.0;
    }

    auto v = vertical_wheel_formula_.value();
    if (std::isnan(v)) {
      logger::get_logger()->error("game_pad_stick_converter vertical_wheel_formula returns nan: {0}",
                                  vertical_wheel_formula_string_);
      v = 0.0;
    }

    return std::make_pair(h, -v);
  }

  void update_continued_movement_timer(continued_movement_mode mode,
                                       std::chrono::milliseconds interval) {
    if (interval == std::chrono::milliseconds(0)) {
      if (continued_movement_mode_ == continued_movement_mode::none) {
        post_event(mode);

      } else if (!xy_.continued_movement() &&
                 !wheels_.continued_movement()) {
        // Stop continued_movement when both the xy stick and wheels stick are not in the continued movement position.​
        continued_movement_mode_ = continued_movement_mode::none;
        continued_movement_timer_.stop();
      }

    } else {
      if (!continued_movement_timer_.enabled()) {
        continued_movement_mode_ = mode;
        continued_movement_timer_count_ = 0;

        continued_movement_timer_.start(
            [this, interval] {
              ++continued_movement_timer_count_;
              switch (continued_movement_timer_count_) {
                case 1:
                  // Ignore immediately fire after start.
                  return;
                case 2:
                  post_event(continued_movement_mode_);
                  continued_movement_timer_.set_interval(interval);
                  break;
                default:
                  post_event(continued_movement_mode_);
                  break;
              }
            },
            // TODO: Replace hard-coded interval
            std::chrono::milliseconds(300));
      }
    }
  }

  void post_event(continued_movement_mode mode) {
    xy_radian_ = xy_.get_radian();
    xy_delta_magnitude_ = xy_.get_delta_magnitude();
    xy_absolute_magnitude_ = xy_.get_absolute_magnitude();
    xy_continued_movement_ = (continued_movement_mode_ == continued_movement_mode::xy);
    if (continued_movement_mode_ == continued_movement_mode::xy &&
        xy_.continued_movement()) {
      // Add secondary stick absolute magnitude to magnitudes;
      auto m = wheels_.get_absolute_magnitude();
      xy_delta_magnitude_ += m;
      xy_absolute_magnitude_ += m;
    }

    wheels_radian_ = wheels_.get_radian();
    wheels_delta_magnitude_ = wheels_.get_delta_magnitude();
    wheels_absolute_magnitude_ = wheels_.get_absolute_magnitude();
    wheels_continued_movement_ = (continued_movement_mode_ == continued_movement_mode::wheels);
    if (continued_movement_mode_ == continued_movement_mode::wheels &&
        wheels_.continued_movement()) {
      // Add secondary stick absolute magnitude to magnitudes;
      auto m = xy_.get_absolute_magnitude();
      wheels_delta_magnitude_ += m;
      wheels_absolute_magnitude_ += m;
    }

    auto [x, y] = xy_hid_values();
    x_value_.set_value(x);
    y_value_.set_value(y);

    auto [h, v] = wheels_hid_values();
    horizontal_wheel_value_.set_value(h);
    vertical_wheel_value_.set_value(v);

    switch (mode) {
      case continued_movement_mode::none:
        break;

      case continued_movement_mode::xy:
        post_xy_event();
        break;

      case continued_movement_mode::wheels:
        post_wheels_event();
        break;
    }
  }

  void post_xy_event(void) {
    pointing_motion m(x_value_.truncated_value(),
                      y_value_.truncated_value(),
                      0,
                      0);

    if (m.is_zero()) {
      return;
    }

    event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
    event_queue::event event(m);
    event_queue::entry entry(device_properties_->get_device_id(),
                             event_time_stamp,
                             event,
                             event_type::single,
                             event,
                             event_queue::state::original);

    enqueue_to_dispatcher([this, entry] {
      pointing_motion_arrived(entry);
    });
  }

  void post_wheels_event(void) {
    pointing_motion m(0,
                      0,
                      vertical_wheel_value_.truncated_value(),
                      horizontal_wheel_value_.truncated_value());

    if (m.is_zero()) {
      return;
    }

    event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
    event_queue::event event(m);
    event_queue::entry entry(device_properties_->get_device_id(),
                             event_time_stamp,
                             event,
                             event_type::single,
                             event,
                             event_queue::state::original);

    enqueue_to_dispatcher([this, entry] {
      pointing_motion_arrived(entry);
    });
  }

  gsl::not_null<std::shared_ptr<device_properties>> device_properties_;
  gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration_;

  stick xy_;
  stick wheels_;

  event_value x_value_;
  event_value y_value_;

  event_value horizontal_wheel_value_;
  event_value vertical_wheel_value_;

  pqrs::dispatcher::extra::timer continued_movement_timer_;
  int continued_movement_timer_count_;
  continued_movement_mode continued_movement_mode_;

  std::string x_formula_string_;
  std::string y_formula_string_;
  std::string vertical_wheel_formula_string_;
  std::string horizontal_wheel_formula_string_;
  exprtk_utility::expression_wrapper x_formula_;
  exprtk_utility::expression_wrapper y_formula_;
  exprtk_utility::expression_wrapper vertical_wheel_formula_;
  exprtk_utility::expression_wrapper horizontal_wheel_formula_;

  double xy_radian_;
  double xy_delta_magnitude_;
  double xy_absolute_magnitude_;
  double xy_continued_movement_;
  double wheels_radian_;
  double wheels_delta_magnitude_;
  double wheels_absolute_magnitude_;
  double wheels_continued_movement_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
