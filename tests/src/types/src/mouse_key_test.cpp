#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("mouse_key") {
  {
    krbn::mouse_key mouse_key(10, 20, 30, 40, 1.0);
    REQUIRE(mouse_key.get_x() == 10);
    REQUIRE(mouse_key.get_y() == 20);
    REQUIRE(mouse_key.get_vertical_wheel() == 30);
    REQUIRE(mouse_key.get_horizontal_wheel() == 40);
    REQUIRE(mouse_key.get_speed_multiplier() == Approx(1.0));

    nlohmann::json json;
    json["x"] = 10;
    json["y"] = 20;
    json["vertical_wheel"] = 30;
    json["horizontal_wheel"] = 40;
    json["speed_multiplier"] = 1.0;

    REQUIRE(mouse_key.to_json() == json);
  }
  {
    nlohmann::json json;
    krbn::mouse_key mouse_key(json);
    REQUIRE(mouse_key.get_x() == 0);
    REQUIRE(mouse_key.get_y() == 0);
    REQUIRE(mouse_key.get_vertical_wheel() == 0);
    REQUIRE(mouse_key.get_horizontal_wheel() == 0);
    REQUIRE(mouse_key.get_speed_multiplier() == Approx(1.0));
  }
  {
    nlohmann::json json;
    json["x"] = 10;
    json["y"] = 20;
    json["vertical_wheel"] = 30;
    json["horizontal_wheel"] = 40;
    json["speed_multiplier"] = 1.0;

    krbn::mouse_key mouse_key(json);
    REQUIRE(mouse_key.get_x() == 10);
    REQUIRE(mouse_key.get_y() == 20);
    REQUIRE(mouse_key.get_vertical_wheel() == 30);
    REQUIRE(mouse_key.get_horizontal_wheel() == 40);
    REQUIRE(mouse_key.get_speed_multiplier() == Approx(1.0));

    REQUIRE(mouse_key.to_json() == json);
  }
  {
    krbn::mouse_key mouse_key1(10, 20, 30, 40, 1.0);
    krbn::mouse_key mouse_key2(1, 2, 3, 4, 2.0);
    krbn::mouse_key mouse_key = mouse_key1 + mouse_key2;
    REQUIRE(mouse_key.get_x() == 11);
    REQUIRE(mouse_key.get_y() == 22);
    REQUIRE(mouse_key.get_vertical_wheel() == 33);
    REQUIRE(mouse_key.get_horizontal_wheel() == 44);
    REQUIRE(mouse_key.get_speed_multiplier() == Approx(2.0));
  }
  {
    REQUIRE(krbn::mouse_key(0, 0, 0, 0, 1.0).is_zero());
    REQUIRE(!krbn::mouse_key(1, 0, 0, 0, 1.0).is_zero());
    REQUIRE(!krbn::mouse_key(0, 2, 0, 0, 1.0).is_zero());
    REQUIRE(!krbn::mouse_key(0, 0, 3, 0, 1.0).is_zero());
    REQUIRE(!krbn::mouse_key(0, 0, 0, 4, 1.0).is_zero());
  }
}

TEST_CASE("mouse_key operators") {
  krbn::mouse_key mouse_key1(10, 20, 30, 40, 1.0);
  krbn::mouse_key mouse_key2 = mouse_key1;
  krbn::mouse_key mouse_key3(1, 2, 3, 4, 2.0);
  REQUIRE(mouse_key1 == mouse_key2);
  REQUIRE(mouse_key1 != mouse_key3);
}
