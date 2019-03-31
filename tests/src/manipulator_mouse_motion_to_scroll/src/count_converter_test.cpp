#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/count_converter.hpp"

TEST_CASE("count_converter::update") {
  krbn::manipulator::manipulators::mouse_motion_to_scroll::count_converter c(8);

  REQUIRE(c.update(0) == 0);
  REQUIRE(c.update(8) == 2); // Multiplied until count is over threshold.
  REQUIRE(c.update(8) == 1);
  REQUIRE(c.update(16) == 2);

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 7; ++j) {
      REQUIRE(c.update(1) == 0);
    }
    REQUIRE(c.update(1) == 1);
  }

  c.reset();

  REQUIRE(c.update(-8) == -2); // Multiplied until count is over threshold.
  REQUIRE(c.update(-8) == -1);
  REQUIRE(c.update(-16) == -2);

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 7; ++j) {
      REQUIRE(c.update(-1) == 0);
    }
    REQUIRE(c.update(-1) == -1);
  }

  c.reset();

  REQUIRE(c.update(4) == 1);
  REQUIRE(c.update(7) == 0);
  REQUIRE(c.update(-1) == 0); // Reset if sign is different.
  REQUIRE(c.update(2) == 0);
}

TEST_CASE("count_converter::make_momentum_value") {
  krbn::manipulator::manipulators::mouse_motion_to_scroll::count_converter c(8);

  REQUIRE(c.update_momentum() == std::nullopt);

  {
    c.reset();
    c.update(16);

    std::vector<int> expected{
        0, 1, 1, 0, // 0-3
        1, 0, 0, 0, // 4-7
        0, 0,       // 8-9
    };
    std::vector<int> actual;
    while (true) {
      auto v = c.update_momentum();
      if (!v) {
        break;
      }
      actual.push_back(*v);
    }

    REQUIRE(actual == expected);
  }

  // Fill zero before update

  {
    c.reset();
    for (int i = 0; i < 8; ++i) {
      c.update(0);
    }

    REQUIRE(c.update_momentum() == std::nullopt);
  }
}
