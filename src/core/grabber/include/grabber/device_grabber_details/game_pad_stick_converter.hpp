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
// This conversion is necessary because it is difficult to use game pad sticks as natural pointing devices due to the following characteristics.
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
    //
    // Signals (invoked from the dispatcher thread)
    //

    nod::signal<void(double horizontal_hid_value,
                     double vertical_hid_value,
                     std::chrono::milliseconds interval,
                     event_origin event_origin)>
        hid_values_updated;

    //
    // Classes
    //

    enum class stick_type {
      xy,
      wheels,
    };

    //
    // Methods
    //

    stick(stick_type stick_type)
        : dispatcher_client(),
          stick_type_(stick_type),
          update_timer_(*this),
          delta_magnitude_(0.0),
          continued_movement_magnitude_(0.0),
          previous_horizontal_value_(0.0),
          previous_vertical_value_(0.0),
          previous_magnitude_(0.0),
          event_origin_(event_origin::none),
          continued_movement_absolute_magnitude_threshold_(1.0) {
      auto update_timer_interval_milliseconds = std::chrono::milliseconds(20);

      update_timer_.start(
          [this] {
            update_values();
          },
          update_timer_interval_milliseconds);
    }

    ~stick(void) {
      detach_from_dispatcher([this] {
        update_timer_.stop();
      });
    }

    void update_horizontal_stick_sensor_value(CFIndex logical_max,
                                              CFIndex logical_min,
                                              CFIndex integer_value,
                                              event_origin event_origin) {
      event_origin_ = event_origin;

      horizontal_stick_sensor_.update_stick_sensor_value(logical_max,
                                                         logical_min,
                                                         integer_value);
    }

    void update_vertical_stick_sensor_value(CFIndex logical_max,
                                            CFIndex logical_min,
                                            CFIndex integer_value,
                                            event_origin event_origin) {
      event_origin_ = event_origin;

      vertical_stick_sensor_.update_stick_sensor_value(logical_max,
                                                       logical_min,
                                                       integer_value);
    }

    void update_configurations(const core_configuration::core_configuration& core_configuration,
                               const device_identifiers& device_identifiers) {
      switch (stick_type_) {
        case stick_type::xy:
          continued_movement_absolute_magnitude_threshold_ = core_configuration.get_selected_profile().get_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(device_identifiers);
          break;
        case stick_type::wheels:
          continued_movement_absolute_magnitude_threshold_ = core_configuration.get_selected_profile().get_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(device_identifiers);
          break;
      }

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

  private:
    // This method is executed in the shared dispatcher thread.
    void update_values(void) {
      auto delta_vertical = vertical_stick_sensor_.get_value() - previous_vertical_value_;
      auto delta_horizontal = horizontal_stick_sensor_.get_value() - previous_horizontal_value_;
      delta_magnitude_ = std::min(1.0,
                                  std::sqrt(
                                      std::pow(delta_horizontal, 2) +
                                      std::pow(delta_vertical, 2)));

      delta_magnitude_history_.push_back(delta_magnitude_);
      // Drop history entries before 100 ms.
      while (delta_magnitude_history_.size() > 5) {
        delta_magnitude_history_.pop_front();
      }

      radian_ = std::atan2(vertical_stick_sensor_.get_value(),
                           horizontal_stick_sensor_.get_value());

      auto magnitude = std::min(1.0,
                                std::sqrt(std::pow(vertical_stick_sensor_.get_value(), 2) +
                                          std::pow(horizontal_stick_sensor_.get_value(), 2)));

      if (magnitude >= continued_movement_absolute_magnitude_threshold_) {
        if (continued_movement_magnitude_ == 0.0) {
          auto it = std::max_element(std::begin(delta_magnitude_history_),
                                     std::end(delta_magnitude_history_));
          continued_movement_magnitude_ = *it;
        }

        delta_magnitude_ = continued_movement_magnitude_;
      } else {
        continued_movement_magnitude_ = 0.0;
      }

      auto delta_magnitude_threshold = 0.01;
      if (delta_magnitude_ < delta_magnitude_threshold) {
        return;
      }

      //
      // Signal
      //

      if (magnitude >= previous_magnitude_) {
        switch (stick_type_) {
          case stick_type::xy: {
            auto [x, y] = xy_hid_values();
            auto interval = xy_stick_interval();
            hid_values_updated(x, y, interval, event_origin_);
            break;
          }
          case stick_type::wheels: {
            auto [h, v] = wheels_hid_values();
            auto interval = wheels_stick_interval();
            hid_values_updated(h, -v, interval, event_origin_);
            break;
          }
        }
      } else {
        hid_values_updated(0, 0, std::chrono::milliseconds(0), event_origin_);
      }

      //
      // Update previous values
      //

      previous_vertical_value_ = vertical_stick_sensor_.get_value();
      previous_horizontal_value_ = horizontal_stick_sensor_.get_value();
      previous_magnitude_ = magnitude;
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

    std::chrono::milliseconds xy_stick_interval(void) const {
      if (continued_movement_magnitude_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      auto interval = xy_stick_interval_milliseconds_formula_.value();
      if (std::isnan(interval)) {
        logger::get_logger()->error("game_pad_stick_converter xy_stick_interval_milliseconds_formula_ returns nan: {0} (radian: {1}, magnitude: {2})",
                                    xy_stick_interval_milliseconds_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        interval = 0;
      }

      return std::chrono::milliseconds(static_cast<int>(interval));
    }

    std::chrono::milliseconds wheels_stick_interval(void) const {
      if (continued_movement_magnitude_ == 0.0) {
        return std::chrono::milliseconds(0);
      }

      auto interval = wheels_stick_interval_milliseconds_formula_.value();
      if (std::isnan(interval)) {
        logger::get_logger()->error("game_pad_stick_converter wheels_stick_interval_milliseconds_formula_ returns nan: {0} (radian: {1}, magnitude: {2})",
                                    wheels_stick_interval_milliseconds_formula_string_,
                                    radian_,
                                    delta_magnitude_);
        interval = 0;
      }

      return std::chrono::milliseconds(static_cast<int>(interval));
    }

    exprtk_utility::expression_t make_formula_expression(const std::string& formula) {
      return exprtk_utility::compile(formula,
                                     {},
                                     {
                                         {"radian", radian_},
                                         {"magnitude", delta_magnitude_},
                                     });
    }

    stick_type stick_type_;

    pqrs::dispatcher::extra::timer update_timer_;

    stick_sensor horizontal_stick_sensor_;
    stick_sensor vertical_stick_sensor_;

    double radian_;
    double delta_magnitude_;
    double continued_movement_magnitude_;
    double previous_horizontal_value_;
    double previous_vertical_value_;
    double previous_magnitude_;
    std::deque<double> delta_magnitude_history_;
    std::atomic<event_origin> event_origin_;

    //
    // configurations
    //

    double continued_movement_absolute_magnitude_threshold_;
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
          xy_(stick::stick_type::xy),
          wheels_(stick::stick_type::wheels),
          x_remainder_(0.0),
          y_remainder_(0.0),
          horizontal_wheel_remainder_(0.0),
          vertical_wheel_remainder_(0.0),
          xy_timer_(*this),
          wheels_timer_(*this) {
      xy_.hid_values_updated.connect([this](auto&& x, auto&& y, auto&& interval, auto&& event_origin) {
        if (interval == std::chrono::milliseconds(0)) {
          xy_timer_.stop();
          post_xy_event(x, y, event_origin);

        } else {
          xy_timer_.start(
              [this, x, y, event_origin] {
                post_xy_event(x, y, event_origin);
              },
              interval);
        }
      });

      wheels_.hid_values_updated.connect([this](auto&& h, auto&& v, auto&& interval, auto&& event_origin) {
        if (interval == std::chrono::milliseconds(0)) {
          wheels_timer_.stop();
          post_wheels_event(h, v, event_origin);

        } else {
          wheels_timer_.start(
              [this, h, v, event_origin] {
                post_wheels_event(h, v, event_origin);
              },
              interval);
        }
      });
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
    }

    void update_x_stick_sensor_value(CFIndex logical_max,
                                     CFIndex logical_min,
                                     CFIndex integer_value,
                                     event_origin event_origin) {
      xy_.update_horizontal_stick_sensor_value(logical_max,
                                               logical_min,
                                               integer_value,
                                               event_origin);
    }

    void update_y_stick_sensor_value(CFIndex logical_max,
                                     CFIndex logical_min,
                                     CFIndex integer_value,
                                     event_origin event_origin) {
      xy_.update_vertical_stick_sensor_value(logical_max,
                                             logical_min,
                                             integer_value,
                                             event_origin);
    }

    void update_vertical_wheel_stick_sensor_value(CFIndex logical_max,
                                                  CFIndex logical_min,
                                                  CFIndex integer_value,
                                                  event_origin event_origin) {
      wheels_.update_vertical_stick_sensor_value(logical_max,
                                                 logical_min,
                                                 integer_value,
                                                 event_origin);
    }

    void update_horizontal_wheel_stick_sensor_value(CFIndex logical_max,
                                                    CFIndex logical_min,
                                                    CFIndex integer_value,
                                                    event_origin event_origin) {
      wheels_.update_horizontal_stick_sensor_value(logical_max,
                                                   logical_min,
                                                   integer_value,
                                                   event_origin);
    }

  private:
    void post_xy_event(double x, double y, event_origin event_origin) {
      auto x_truncated = std::trunc(x + x_remainder_);
      x_remainder_ += x - x_truncated;

      auto y_truncated = std::trunc(y + y_remainder_);
      y_remainder_ += y - y_truncated;

      pointing_motion m(adjust_integer_value(x_truncated),
                        adjust_integer_value(y_truncated),
                        0,
                        0);

      if (m.is_zero()) {
        return;
      }

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
    }

    void post_wheels_event(double horizontal_wheel, double vertical_wheel, event_origin event_origin) {
      auto horizontal_wheel_truncated = std::trunc(horizontal_wheel + horizontal_wheel_remainder_);
      horizontal_wheel_remainder_ += horizontal_wheel - horizontal_wheel_truncated;

      auto vertical_wheel_truncated = std::trunc(vertical_wheel + vertical_wheel_remainder_);
      vertical_wheel_remainder_ += vertical_wheel - vertical_wheel_truncated;

      pointing_motion m(0,
                        0,
                        adjust_integer_value(vertical_wheel_truncated),
                        adjust_integer_value(horizontal_wheel_truncated));

      if (m.is_zero()) {
        return;
      }

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
    }

    int adjust_integer_value(double truncated_value) const {
      auto value = static_cast<int>(truncated_value);

      // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
      value = std::min(value, 127);
      value = std::max(value, -127);
      return value;
    }

    device_id device_id_;
    device_identifiers device_identifiers_;
    const pointing_motion_arrived_t& pointing_motion_arrived_;

    stick xy_;
    stick wheels_;

    double x_remainder_;
    double y_remainder_;
    double horizontal_wheel_remainder_;
    double vertical_wheel_remainder_;

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
                                                                           v.get_integer_value(),
                                                                           event_origin);
                  } else {
                    it->second->update_x_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value(),
                                                            event_origin);
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::y)) {
                  if (swap_sticks) {
                    it->second->update_vertical_wheel_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value(),
                                                                         event_origin);
                  } else {
                    it->second->update_y_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value(),
                                                            event_origin);
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::rz)) {
                  if (swap_sticks) {
                    it->second->update_y_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value(),
                                                            event_origin);
                  } else {
                    it->second->update_vertical_wheel_stick_sensor_value(*logical_max,
                                                                         *logical_min,
                                                                         v.get_integer_value(),
                                                                         event_origin);
                  }
                } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                         pqrs::hid::usage::generic_desktop::z)) {
                  if (swap_sticks) {
                    it->second->update_x_stick_sensor_value(*logical_max,
                                                            *logical_min,
                                                            v.get_integer_value(),
                                                            event_origin);
                  } else {
                    it->second->update_horizontal_wheel_stick_sensor_value(*logical_max,
                                                                           *logical_min,
                                                                           v.get_integer_value(),
                                                                           event_origin);
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
  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  std::unordered_map<device_id, std::shared_ptr<state>> states_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
