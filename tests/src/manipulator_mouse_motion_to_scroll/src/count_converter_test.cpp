#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/count_converter.hpp"

TEST_CASE("count_converter::update") {
  krbn::manipulator::manipulators::mouse_motion_to_scroll::count_converter c(8);

  REQUIRE(c.update(0) == 0);
  REQUIRE(c.update(8) == 1);
  REQUIRE(c.update(16) == 2);

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 7; ++j) {
      REQUIRE(c.update(1) == 0);
    }
    REQUIRE(c.update(1) == 1);
  }

  c.reset();

  REQUIRE(c.update(-8) == -1);
  REQUIRE(c.update(-16) == -2);

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 7; ++j) {
      REQUIRE(c.update(-1) == 0);
    }
    REQUIRE(c.update(-1) == -1);
  }
}

TEST_CASE("count_converter::make_momentum_value") {
  krbn::manipulator::manipulators::mouse_motion_to_scroll::count_converter c(8);

  REQUIRE(c.update_momentum() == 0);

  {
    c.reset();
    c.update(16);

    std::vector<int> expected{1, 1, 1, 1,
                              0, 1, 0, 0};
    std::vector<int> actual;
    for (int i = 0; i < 8; ++i) {
      actual.push_back(c.update_momentum());
    }
    REQUIRE(actual == expected);
  }

  // Fill zero before update

  {
    c.reset();
    for (int i = 0; i < 8; ++i) {
      c.update(0);
    }

    std::vector<int> expected{0, 0, 0, 0,
                              0, 0, 0, 0};
    std::vector<int> actual;
    for (int i = 0; i < 8; ++i) {
      actual.push_back(c.update_momentum());
    }
    REQUIRE(actual == expected);
  }
}
