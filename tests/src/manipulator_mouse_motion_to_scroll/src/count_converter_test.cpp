#include <catch2/catch.hpp>

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

  REQUIRE(c.make_momentum_value() == 0);

  {
    c.reset();
    c.update(16);

    std::vector<int> values{8, 6, 5, 4, 3, 3, 3, 3, 2, 1, 1, 1, 1, 0};
    for (const auto v : values) {
      REQUIRE(c.make_momentum_value() == v);
      c.update(c.make_momentum_value());
    }
  }

  // Fill zero before update

  {
    c.reset();
    for (int i = 0; i < 8; ++i) {
      c.update(0);
    }

    c.update(8);
    REQUIRE(c.make_momentum_value() == 0);

    c.update(8);
    REQUIRE(c.make_momentum_value() == 1);

    c.update(8);
    REQUIRE(c.make_momentum_value() == 1);

    c.update(8);
    REQUIRE(c.make_momentum_value() == 2);
  }
}
