#pragma once

#include "logger.hpp"
#include "types/absolute_time_duration.hpp"
#include <pqrs/osx/chrono.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
struct counter_parameters final {
public:
  static constexpr double speed_multiplier_default_value = 1.0;
  static constexpr std::chrono::milliseconds recent_time_duration_milliseconds_default_value =
      std::chrono::milliseconds(100);
  static constexpr int threshold_default_value = 128;
  static constexpr int direction_lock_threshold_default_value = 4;
  static constexpr std::chrono::milliseconds scroll_event_interval_milliseconds_threshold_default_value =
      std::chrono::milliseconds(100);

  counter_parameters(void) : momentum_scroll_enabled_(true),
                             speed_multiplier_(speed_multiplier_default_value),
                             recent_time_duration_milliseconds_(recent_time_duration_milliseconds_default_value),
                             threshold_(threshold_default_value),
                             direction_lock_threshold_(direction_lock_threshold_default_value),
                             scroll_event_interval_milliseconds_threshold_(scroll_event_interval_milliseconds_threshold_default_value) {
  }

  bool get_momentum_scroll_enabled(void) const {
    return momentum_scroll_enabled_;
  }

  void set_momentum_scroll_enabled(bool value) {
    momentum_scroll_enabled_ = value;
  }

  double get_speed_multiplier(void) const {
    return speed_multiplier_;
  }

  void set_speed_multiplier(double value) {
    if (value <= 0.0) {
      value = speed_multiplier_default_value;
    }

    speed_multiplier_ = value;
  }

  std::chrono::milliseconds get_recent_time_duration_milliseconds(void) const {
    return recent_time_duration_milliseconds_;
  }

  void set_recent_time_duration_milliseconds(std::chrono::milliseconds value) {
    if (value < std::chrono::milliseconds(1)) {
      value = recent_time_duration_milliseconds_default_value;
    }

    recent_time_duration_milliseconds_ = value;
  }

  int get_threshold(void) const {
    return threshold_;
  }

  void set_threshold(int value) {
    if (value <= 0) {
      value = threshold_default_value;
    }

    threshold_ = value;
  }

  int get_direction_lock_threshold(void) const {
    return direction_lock_threshold_;
  }

  void set_direction_lock_threshold(int value) {
    if (value <= 0) {
      value = direction_lock_threshold_default_value;
    }

    direction_lock_threshold_ = value;
  }

  std::chrono::milliseconds get_scroll_event_interval_milliseconds_threshold(void) const {
    return scroll_event_interval_milliseconds_threshold_;
  }

  void set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds value) {
    if (value < std::chrono::milliseconds(1)) {
      value = scroll_event_interval_milliseconds_threshold_default_value;
    }

    scroll_event_interval_milliseconds_threshold_ = value;
  }

  void update(const nlohmann::json& json) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      if (key == "momentum_scroll_enabled") {
        if (!value.is_boolean()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", key, value.dump()));
        }

        set_momentum_scroll_enabled(value.get<bool>());

      } else if (key == "speed_multiplier") {
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        set_speed_multiplier(value.get<double>());

      } else if (key == "threshold") { // (secret parameter)
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        set_threshold(value.get<int>());

      } else if (key == "recent_time_duration_milliseconds") { // (secret parameter)
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        set_recent_time_duration_milliseconds(std::chrono::milliseconds(value.get<int>()));

      } else if (key == "direction_lock_threshold") { // (secret parameter)
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        set_direction_lock_threshold(value.get<int>());

      } else if (key == "scroll_event_interval_milliseconds_threshold") { // (secret parameter)
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(value.get<int>()));
      }
    }
  }

private:
  bool momentum_scroll_enabled_;
  double speed_multiplier_;
  std::chrono::milliseconds recent_time_duration_milliseconds_;
  int threshold_;
  int direction_lock_threshold_;
  std::chrono::milliseconds scroll_event_interval_milliseconds_threshold_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
