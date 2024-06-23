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

    nod::signal<void(double horizontal_hid_value,
                     double vertical_hid_value,
                     std::chrono::milliseconds interval)>
        hid_values_updated;

    //
    // Classes
    //

    enum class stick_type {
      xy,
      wheels,
    };

    class stick_history_entry final {
    public:
      stick_history_entry(absolute_time_point time,
                          double horizontal_stick_sensor_value,
                          double vertical_stick_sensor_value)
          : time_(time),
            horizontal_stick_sensor_value_(horizontal_stick_sensor_value),
            vertical_stick_sensor_value_(vertical_stick_sensor_value) {
      }

      absolute_time_point get_time(void) const {
        return time_;
      }

      double get_horizontal_stick_sensor_value(void) const {
        return horizontal_stick_sensor_value_;
      }

      double get_vertical_stick_sensor_value(void) const {
        return vertical_stick_sensor_value_;
      }

    private:
      absolute_time_point time_;
      double horizontal_stick_sensor_value_;
      double vertical_stick_sensor_value_;
    };

    //
    // Methods
    //

    stick(stick_type stick_type)
        : dispatcher_client(),
          stick_type_(stick_type),
          radian_(0.0),
          absolute_magnitude_(0.0),
          delta_magnitude_(0.0),
          continued_movement_magnitude_(0.0),
          previous_absolute_magnitude_(0.0),
          continued_movement_absolute_magnitude_threshold_(1.0),
          flicking_input_window_milliseconds_(0) {
    }

    ~stick(void) {
      detach_from_dispatcher();
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

    void update_configurations(const core_configuration::core_configuration& core_configuration,
                               const device_identifiers& device_identifiers) {
      auto d = core_configuration.get_selected_profile().get_device(device_identifiers);

      switch (stick_type_) {
        case stick_type::xy:
          continued_movement_absolute_magnitude_threshold_ = d->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold();
          continued_movement_interval_milliseconds_ = d->get_game_pad_xy_stick_continued_movement_interval_milliseconds();
          flicking_input_window_milliseconds_ = d->get_game_pad_xy_stick_flicking_input_window_milliseconds();
          break;
        case stick_type::wheels:
          continued_movement_absolute_magnitude_threshold_ = d->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold();
          continued_movement_interval_milliseconds_ = d->get_game_pad_wheels_stick_continued_movement_interval_milliseconds();
          flicking_input_window_milliseconds_ = d->get_game_pad_wheels_stick_flicking_input_window_milliseconds();
          break;
      }

      x_formula_string_ = d->get_game_pad_stick_x_formula();
      y_formula_string_ = d->get_game_pad_stick_y_formula();
      vertical_wheel_formula_string_ = d->get_game_pad_stick_vertical_wheel_formula();
      horizontal_wheel_formula_string_ = d->get_game_pad_stick_horizontal_wheel_formula();

      x_formula_ = make_formula_expression(x_formula_string_);
      y_formula_ = make_formula_expression(y_formula_string_);
      vertical_wheel_formula_ = make_formula_expression(vertical_wheel_formula_string_);
      horizontal_wheel_formula_ = make_formula_expression(horizontal_wheel_formula_string_);
    }

  private:
    // This method is executed in the shared dispatcher thread.
    void update_values(void) {
      auto now = pqrs::osx::chrono::mach_absolute_time_point();

      radian_ = std::atan2(vertical_stick_sensor_.get_value(),
                           horizontal_stick_sensor_.get_value());
      // When the stick is tilted diagonally, the distance from the centre may exceed 1.0 (e.g. 1.2).
      // Therefore, we correct the value <= 1.0.
      absolute_magnitude_ = std::min(1.0,
                                     std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                               std::pow(horizontal_stick_sensor_.get_value(), 2)));

      auto dm = std::max(0.0, absolute_magnitude_ - previous_absolute_magnitude_);

      //
      // Update stick_history_
      //

      stick_history_.push_back(stick_history_entry(now,
                                                   horizontal_stick_sensor_.get_value(),
                                                   vertical_stick_sensor_.get_value()));
      stick_history_.erase(std::remove_if(std::begin(stick_history_),
                                          std::end(stick_history_),
                                          [this, now](auto&& e) {
                                            return pqrs::osx::chrono::make_milliseconds(now - e.get_time()) > std::chrono::milliseconds(flicking_input_window_milliseconds_);
                                          }),
                           std::end(stick_history_));

      //
      // Update delta_magnitude_
      //

      if (absolute_magnitude_ >= continued_movement_absolute_magnitude_threshold_) {
        if (continued_movement_magnitude_ == 0.0) {
          double max = 0;
          for (int i = 0; i < stick_history_.size(); ++i) {
            for (int j = i + 1; j < stick_history_.size(); ++j) {
              auto h = stick_history_[i].get_horizontal_stick_sensor_value() - stick_history_[j].get_horizontal_stick_sensor_value();
              auto v = stick_history_[i].get_vertical_stick_sensor_value() - stick_history_[j].get_vertical_stick_sensor_value();
              auto d = std::sqrt(h * h + v * v);
              if (max < d) {
                max = d;
              }
            }
          }
          continued_movement_magnitude_ = max;
        }

        delta_magnitude_ = continued_movement_magnitude_;
      } else {
        continued_movement_magnitude_ = 0.0;
        delta_magnitude_ = dm;
      }

      //
      // Signal
      //

      if (delta_magnitude_ > 0) {
        auto delta_magnitude_threshold = 0.01;
        if (delta_magnitude_ < delta_magnitude_threshold) {
          // Return in here to prevent previous_absolute_magnitude_ from being updated.
          return;
        }

        switch (stick_type_) {
          case stick_type::xy: {
            auto [x, y] = xy_hid_values();
            auto interval = continued_movement_interval_milliseconds();
            hid_values_updated(x, y, interval);
            break;
          }
          case stick_type::wheels: {
            auto [h, v] = wheels_hid_values();
            auto interval = continued_movement_interval_milliseconds();
            hid_values_updated(h, -v, interval);
            break;
          }
        }
      } else {
        hid_values_updated(0, 0, std::chrono::milliseconds(0));
      }

      //
      // Update previous values
      //

      previous_absolute_magnitude_ = absolute_magnitude_;
    }

    std::pair<double, double> xy_hid_values(void) const {
      auto x = x_formula_.value();
      if (std::isnan(x)) {
        logger::get_logger()->error("game_pad_stick_converter x_formula returns nan: {0} (radian: {1}, magnitude: {2})",
                                    x_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        x = 0.0;
      }

      auto y = y_formula_.value();
      if (std::isnan(y)) {
        logger::get_logger()->error("game_pad_stick_converter y_formula returns nan: {0} (radian: {1}, magnitude: {2})",
                                    y_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        y = 0.0;
      }

      return std::make_pair(x, y);
    }

    std::pair<double, double> wheels_hid_values(void) const {
      auto h = horizontal_wheel_formula_.value();
      if (std::isnan(h)) {
        logger::get_logger()->error("game_pad_stick_converter horizontal_wheel_formula returns nan: {0} (radian: {1}, magnitude: {2})",
                                    horizontal_wheel_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        h = 0.0;
      }

      auto v = vertical_wheel_formula_.value();
      if (std::isnan(v)) {
        logger::get_logger()->error("game_pad_stick_converter vertical_wheel_formula returns nan: {0} (radian: {1}, magnitude: {2})",
                                    vertical_wheel_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        v = 0.0;
      }

      return std::make_pair(h, v);
    }

    std::chrono::milliseconds continued_movement_interval_milliseconds(void) const {
      if (continued_movement_magnitude_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      return std::chrono::milliseconds(continued_movement_interval_milliseconds_);
    }

    exprtk_utility::expression_t make_formula_expression(const std::string& formula) {
      return exprtk_utility::compile(formula,
                                     {},
                                     {
                                         {"radian", radian_},
                                         {"delta_magnitude", delta_magnitude_},
                                         {"absolute_magnitude", absolute_magnitude_},
                                     });
    }

    stick_type stick_type_;

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;

    double radian_;
    double absolute_magnitude_;
    double delta_magnitude_;
    double continued_movement_magnitude_;
    double previous_absolute_magnitude_;
    std::deque<stick_history_entry> stick_history_;

    //
    // configurations
    //

    double continued_movement_absolute_magnitude_threshold_;
    int continued_movement_interval_milliseconds_;
    int flicking_input_window_milliseconds_;
    std::string x_formula_string_;
    std::string y_formula_string_;
    std::string vertical_wheel_formula_string_;
    std::string horizontal_wheel_formula_string_;
    exprtk_utility::expression_t x_formula_;
    exprtk_utility::expression_t y_formula_;
    exprtk_utility::expression_t vertical_wheel_formula_;
    exprtk_utility::expression_t horizontal_wheel_formula_;
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

  game_pad_stick_converter(const device_properties& device_properties,
                           std::weak_ptr<const core_configuration::core_configuration> weak_core_configuration)
      : dispatcher_client(),
        device_properties_(device_properties),
        xy_(stick::stick_type::xy),
        wheels_(stick::stick_type::wheels),
        xy_timer_(*this),
        xy_timer_count_(0),
        wheels_timer_(*this),
        wheels_timer_count_(0) {
    set_weak_core_configuration(weak_core_configuration);

    xy_.hid_values_updated.connect([this](auto&& x, auto&& y, auto&& interval) {
      x_value_.set_value(x);
      y_value_.set_value(y);

      if (interval == std::chrono::milliseconds(0)) {
        xy_timer_.stop();
        post_xy_event();

      } else {
        if (!xy_timer_.enabled()) {
          xy_timer_count_ = 0;

          xy_timer_.start(
              [this, interval] {
                ++xy_timer_count_;
                switch (xy_timer_count_) {
                  case 1:
                    // Ignore immediately fire after start.
                    return;
                  case 2:
                    post_xy_event();
                    xy_timer_.set_interval(interval);
                    break;
                  default:
                    post_xy_event();
                    break;
                }
              },
              // TODO: Replace hard-coded interval
              std::chrono::milliseconds(300));
        }
      }
    });

    wheels_.hid_values_updated.connect([this](auto&& h, auto&& v, auto&& interval) {
      horizontal_wheel_value_.set_value(h);
      vertical_wheel_value_.set_value(v);

      if (interval == std::chrono::milliseconds(0)) {
        wheels_timer_.stop();
        post_wheels_event();

      } else {
        if (!wheels_timer_.enabled()) {
          wheels_timer_count_ = 0;

          wheels_timer_.start(
              [this, interval] {
                ++wheels_timer_count_;
                switch (wheels_timer_count_) {
                  case 1:
                    // Ignore immediately fire after start.
                    return;
                  case 2:
                    post_wheels_event();
                    wheels_timer_.set_interval(interval);
                    break;
                  default:
                    post_wheels_event();
                    break;
                }
              },
              // TODO: Replace hard-coded interval
              std::chrono::milliseconds(300));
        }
      }
    });
  }

  ~game_pad_stick_converter(void) {
    detach_from_dispatcher([this] {
      xy_timer_.stop();
      wheels_timer_.stop();
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

  void set_weak_core_configuration(std::weak_ptr<const core_configuration::core_configuration> weak_core_configuration) {
    weak_core_configuration_ = weak_core_configuration;

    if (auto c = weak_core_configuration_.lock()) {
      update_configurations(*c);
    }
  }

  // This method should be called in the shared dispatcher thread.
  void convert(const std::vector<pqrs::osx::iokit_hid_value>& hid_values) {
    if (auto d = weak_dispatcher_.lock()) {
      if (!d->dispatcher_thread()) {
        logger::get_logger()->error("game_pad_stick_converter::convert is called in invalid thread");
        return;
      }
    }

    auto c = weak_core_configuration_.lock();
    if (!c) {
      return;
    }

    auto device = c->get_selected_profile().get_device(device_properties_.get_device_identifiers());
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
  void update_configurations(const core_configuration::core_configuration& core_configuration) {
    xy_.update_configurations(core_configuration,
                              device_properties_.get_device_identifiers());
    wheels_.update_configurations(core_configuration,
                                  device_properties_.get_device_identifiers());
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
    event_queue::entry entry(device_properties_.get_device_id(),
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
    event_queue::entry entry(device_properties_.get_device_id(),
                             event_time_stamp,
                             event,
                             event_type::single,
                             event,
                             event_queue::state::original);

    enqueue_to_dispatcher([this, entry] {
      pointing_motion_arrived(entry);
    });
  }

  device_properties device_properties_;
  std::weak_ptr<const core_configuration::core_configuration> weak_core_configuration_;

  stick xy_;
  stick wheels_;

  event_value x_value_;
  event_value y_value_;

  event_value horizontal_wheel_value_;
  event_value vertical_wheel_value_;

  pqrs::dispatcher::extra::timer xy_timer_;
  int xy_timer_count_;

  pqrs::dispatcher::extra::timer wheels_timer_;
  int wheels_timer_count_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
