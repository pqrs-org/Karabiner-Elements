#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("pointing_motion") {
  {
    krbn::pointing_motion pointing_motion;
    REQUIRE(pointing_motion.get_x() == 0);
    REQUIRE(pointing_motion.get_y() == 0);
    REQUIRE(pointing_motion.get_vertical_wheel() == 0);
    REQUIRE(pointing_motion.get_horizontal_wheel() == 0);

    nlohmann::json json;
    json["x"] = 0;
    json["y"] = 0;
    json["vertical_wheel"] = 0;
    json["horizontal_wheel"] = 0;
    REQUIRE(nlohmann::json(pointing_motion) == json);
  }

  {
    krbn::pointing_motion pointing_motion(1, 2, 3, 4);
    REQUIRE(pointing_motion.get_x() == 1);
    REQUIRE(pointing_motion.get_y() == 2);
    REQUIRE(pointing_motion.get_vertical_wheel() == 3);
    REQUIRE(pointing_motion.get_horizontal_wheel() == 4);
  }
}

TEST_CASE("pointing_motion::setter,getter") {
  {
    krbn::pointing_motion pointing_motion;

    pointing_motion.set_x(1);
    pointing_motion.set_y(-1);
    pointing_motion.set_vertical_wheel(2);
    pointing_motion.set_horizontal_wheel(-2);

    REQUIRE(pointing_motion.get_x() == 1);
    REQUIRE(pointing_motion.get_y() == -1);
    REQUIRE(pointing_motion.get_vertical_wheel() == 2);
    REQUIRE(pointing_motion.get_horizontal_wheel() == -2);

    nlohmann::json json;
    json["x"] = 1;
    json["y"] = -1;
    json["vertical_wheel"] = 2;
    json["horizontal_wheel"] = -2;
    REQUIRE(nlohmann::json(pointing_motion) == json);
  }
}

TEST_CASE("pointing_motion::from_json") {
  // from_json
  {
    nlohmann::json json;
    json["x"] = 10;
    json["y"] = -10;
    json["vertical_wheel"] = 20;
    json["horizontal_wheel"] = -20;

    krbn::pointing_motion pointing_motion(json);
    REQUIRE(pointing_motion.get_x() == 10);
    REQUIRE(pointing_motion.get_y() == -10);
    REQUIRE(pointing_motion.get_vertical_wheel() == 20);
    REQUIRE(pointing_motion.get_horizontal_wheel() == -20);
  }
}

TEST_CASE("pointing_motion::is_zero") {
  // is_zero
  {
    krbn::pointing_motion pointing_motion;
    REQUIRE(pointing_motion.is_zero());
  }
  {
    krbn::pointing_motion pointing_motion;
    pointing_motion.set_x(1);
    REQUIRE(!pointing_motion.is_zero());
  }
  {
    krbn::pointing_motion pointing_motion;
    pointing_motion.set_y(1);
    REQUIRE(!pointing_motion.is_zero());
  }
  {
    krbn::pointing_motion pointing_motion;
    pointing_motion.set_vertical_wheel(1);
    REQUIRE(!pointing_motion.is_zero());
  }
  {
    krbn::pointing_motion pointing_motion;
    pointing_motion.set_horizontal_wheel(1);
    REQUIRE(!pointing_motion.is_zero());
  }
}

TEST_CASE("pointing_motion::clear") {
  // clear
  {
    krbn::pointing_motion pointing_motion;
    pointing_motion.set_x(1);
    pointing_motion.set_y(1);
    pointing_motion.set_vertical_wheel(1);
    pointing_motion.set_horizontal_wheel(1);
    pointing_motion.clear();
    REQUIRE(pointing_motion.is_zero());
  }
}
