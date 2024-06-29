#pragma once

#include "../../configuration_json_helper.hpp"
#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications_parameters final {
public:
  complex_modifications_parameters(const complex_modifications_parameters&) = delete;

  complex_modifications_parameters(void)
      : complex_modifications_parameters(nlohmann::json::object(),
                                         krbn::core_configuration::error_handling::loose) {
  }

  complex_modifications_parameters(const nlohmann::json& json,
                                   error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<int>("basic.simultaneous_threshold_milliseconds",
                                        basic_simultaneous_threshold_milliseconds_,
                                        50);

    helper_values_.push_back_value<int>("basic.to_if_alone_timeout_milliseconds",
                                        basic_to_if_alone_timeout_milliseconds_,
                                        1000);

    helper_values_.push_back_value<int>("basic.to_if_held_down_threshold_milliseconds",
                                        basic_to_if_held_down_threshold_milliseconds_,
                                        500);

    helper_values_.push_back_value<int>("basic.to_delayed_action_delay_milliseconds",
                                        basic_to_delayed_action_delay_milliseconds_,
                                        500);

    helper_values_.push_back_value<int>("mouse_motion_to_scroll.speed",
                                        mouse_motion_to_scroll_speed_,
                                        100);

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    adjust_values();
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    return j;
  }

  void update(const nlohmann::json& json,
              error_handling error_handling) {
    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    adjust_values();
  }

  const int& get_basic_simultaneous_threshold_milliseconds(void) const {
    return basic_simultaneous_threshold_milliseconds_;
  }

  void set_basic_simultaneous_threshold_milliseconds(int value) {
    basic_simultaneous_threshold_milliseconds_ = value;

    adjust_values();
  }

  const int& get_basic_to_if_alone_timeout_milliseconds(void) const {
    return basic_to_if_alone_timeout_milliseconds_;
  }

  void set_basic_to_if_alone_timeout_milliseconds(int value) {
    basic_to_if_alone_timeout_milliseconds_ = value;

    adjust_values();
  }

  const int& get_basic_to_if_held_down_threshold_milliseconds(void) const {
    return basic_to_if_held_down_threshold_milliseconds_;
  }

  void set_basic_to_if_held_down_threshold_milliseconds(int value) {
    basic_to_if_held_down_threshold_milliseconds_ = value;

    adjust_values();
  }

  const int& get_basic_to_delayed_action_delay_milliseconds(void) const {
    return basic_to_delayed_action_delay_milliseconds_;
  }

  void set_basic_to_delayed_action_delay_milliseconds(int value) {
    basic_to_delayed_action_delay_milliseconds_ = value;

    adjust_values();
  }

  const int& get_mouse_motion_to_scroll_speed(void) const {
    return mouse_motion_to_scroll_speed_;
  }

  void set_mouse_motion_to_scroll_speed(int value) {
    mouse_motion_to_scroll_speed_ = value;

    adjust_values();
  }

  double make_mouse_motion_to_scroll_speed_rate(void) const {
    return static_cast<double>(mouse_motion_to_scroll_speed_) / 100;
  }

private:
  void adjust_values(void) {
    adjust_value(basic_simultaneous_threshold_milliseconds_, 0, 1000);
    adjust_value(basic_to_if_alone_timeout_milliseconds_, 0, std::nullopt);
    adjust_value(basic_to_if_held_down_threshold_milliseconds_, 0, std::nullopt);
    adjust_value(basic_to_delayed_action_delay_milliseconds_, 0, std::nullopt);
    adjust_value(mouse_motion_to_scroll_speed_, 1, 10000);
  }

  void adjust_value(int& value, std::optional<int> min, std::optional<int> max) {
    if (auto v = helper_values_.find_value(value)) {
      if (min) {
        if (value < *min) {
          logger::get_logger()->warn("{0} should be >= {1}.", v->get_key(), *min);
        }
        value = std::max(value, *min);
      }

      if (max) {
        if (value > *max) {
          logger::get_logger()->warn("{0} should be <= {1}.", v->get_key(), *max);
        }
        value = std::min(value, *max);
      }
    }
  }

  nlohmann::json json_;
  int basic_simultaneous_threshold_milliseconds_;
  int basic_to_if_alone_timeout_milliseconds_;
  int basic_to_if_held_down_threshold_milliseconds_;
  int basic_to_delayed_action_delay_milliseconds_;
  int mouse_motion_to_scroll_speed_;
  configuration_json_helper::helper_values helper_values_;
};
} // namespace details
} // namespace core_configuration
} // namespace krbn
