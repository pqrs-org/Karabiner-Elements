#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/counter_parameters.hpp"

namespace mouse_motion_to_scroll = krbn::manipulator::manipulators::mouse_motion_to_scroll;

TEST_CASE("counter_parameters") {
  {
    mouse_motion_to_scroll::counter_parameters counter_parameters;

    // speed_multiplier_

    counter_parameters.set_speed_multiplier(-100);
    REQUIRE(counter_parameters.get_speed_multiplier() == Approx(1.0));

    counter_parameters.set_speed_multiplier(0);
    REQUIRE(counter_parameters.get_speed_multiplier() == Approx(1.0));

    counter_parameters.set_speed_multiplier(0.5);
    REQUIRE(counter_parameters.get_speed_multiplier() == Approx(0.5));

    counter_parameters.set_speed_multiplier(1.5);
    REQUIRE(counter_parameters.get_speed_multiplier() == Approx(1.5));

    // recent_time_duration_milliseconds_

    counter_parameters.set_recent_time_duration_milliseconds(std::chrono::milliseconds(-100));
    REQUIRE(counter_parameters.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(100));

    counter_parameters.set_recent_time_duration_milliseconds(std::chrono::milliseconds(0));
    REQUIRE(counter_parameters.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(100));

    counter_parameters.set_recent_time_duration_milliseconds(std::chrono::milliseconds(1));
    REQUIRE(counter_parameters.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(1));

    counter_parameters.set_recent_time_duration_milliseconds(std::chrono::milliseconds(200));
    REQUIRE(counter_parameters.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(200));

    // threshold_

    counter_parameters.set_threshold(-100);
    REQUIRE(counter_parameters.get_threshold() == 32);

    counter_parameters.set_threshold(0);
    REQUIRE(counter_parameters.get_threshold() == 32);

    counter_parameters.set_threshold(1);
    REQUIRE(counter_parameters.get_threshold() == 1);

    counter_parameters.set_threshold(100);
    REQUIRE(counter_parameters.get_threshold() == 100);

    // momentum_minus_

    counter_parameters.set_momentum_minus(-100);
    REQUIRE(counter_parameters.get_momentum_minus() == 16);

    counter_parameters.set_momentum_minus(0);
    REQUIRE(counter_parameters.get_momentum_minus() == 16);

    counter_parameters.set_momentum_minus(1);
    REQUIRE(counter_parameters.get_momentum_minus() == 1);

    counter_parameters.set_momentum_minus(100);
    REQUIRE(counter_parameters.get_momentum_minus() == 100);
  }
}
