#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/counter_parameters.hpp"

namespace mouse_motion_to_scroll = krbn::manipulator::manipulators::mouse_motion_to_scroll;

TEST_CASE("counter_parameters") {
  using counter_parameters = mouse_motion_to_scroll::counter_parameters;

  {
    counter_parameters p;

    // speed_multiplier_

    p.set_speed_multiplier(-100);
    REQUIRE(p.get_speed_multiplier() == Approx(counter_parameters::speed_multiplier_default_value));

    p.set_speed_multiplier(0);
    REQUIRE(p.get_speed_multiplier() == Approx(counter_parameters::speed_multiplier_default_value));

    p.set_speed_multiplier(0.5);
    REQUIRE(p.get_speed_multiplier() == Approx(0.5));

    p.set_speed_multiplier(1.5);
    REQUIRE(p.get_speed_multiplier() == Approx(1.5));

    // recent_time_duration_milliseconds_

    p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(-100));
    REQUIRE(p.get_recent_time_duration_milliseconds() == counter_parameters::recent_time_duration_milliseconds_default_value);

    p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(0));
    REQUIRE(p.get_recent_time_duration_milliseconds() == counter_parameters::recent_time_duration_milliseconds_default_value);

    p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(1));
    REQUIRE(p.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(1));

    p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(200));
    REQUIRE(p.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(200));

    // threshold_

    p.set_threshold(-100);
    REQUIRE(p.get_threshold() == counter_parameters::threshold_default_value);

    p.set_threshold(0);
    REQUIRE(p.get_threshold() == counter_parameters::threshold_default_value);

    p.set_threshold(1);
    REQUIRE(p.get_threshold() == 1);

    p.set_threshold(1000);
    REQUIRE(p.get_threshold() == 1000);

    // momentum_minus_

    p.set_momentum_minus(-100);
    REQUIRE(p.get_momentum_minus() == counter_parameters::momentum_minus_default_value);

    p.set_momentum_minus(0);
    REQUIRE(p.get_momentum_minus() == counter_parameters::momentum_minus_default_value);

    p.set_momentum_minus(1);
    REQUIRE(p.get_momentum_minus() == 1);

    p.set_momentum_minus(1000);
    REQUIRE(p.get_momentum_minus() == 1000);

    // direction_lock_threshold_

    p.set_direction_lock_threshold(-100);
    REQUIRE(p.get_direction_lock_threshold() == counter_parameters::direction_lock_threshold_default_value);

    p.set_direction_lock_threshold(0);
    REQUIRE(p.get_direction_lock_threshold() == counter_parameters::direction_lock_threshold_default_value);

    p.set_direction_lock_threshold(1);
    REQUIRE(p.get_direction_lock_threshold() == 1);

    p.set_direction_lock_threshold(1000);
    REQUIRE(p.get_direction_lock_threshold() == 1000);

    // scroll_event_interval_milliseconds_threshold_

    p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(-100));
    REQUIRE(p.get_scroll_event_interval_milliseconds_threshold() == counter_parameters::scroll_event_interval_milliseconds_threshold_default_value);

    p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(0));
    REQUIRE(p.get_scroll_event_interval_milliseconds_threshold() == counter_parameters::scroll_event_interval_milliseconds_threshold_default_value);

    p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(1));
    REQUIRE(p.get_scroll_event_interval_milliseconds_threshold() == std::chrono::milliseconds(1));

    p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(1000));
    REQUIRE(p.get_scroll_event_interval_milliseconds_threshold() == std::chrono::milliseconds(1000));
  }
}
