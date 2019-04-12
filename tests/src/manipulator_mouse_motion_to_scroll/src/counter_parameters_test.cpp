#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/counter_parameters.hpp"

namespace mouse_motion_to_scroll = krbn::manipulator::manipulators::mouse_motion_to_scroll;

TEST_CASE("counter_parameters") {
  {
    mouse_motion_to_scroll::counter_parameters counter_parameters;

    counter_parameters.set_threshold(-100);
    REQUIRE(counter_parameters.get_threshold() == 1);

    counter_parameters.set_threshold(0);
    REQUIRE(counter_parameters.get_threshold() == 1);

    counter_parameters.set_threshold(100);
    REQUIRE(counter_parameters.get_threshold() == 100);
  }
}
