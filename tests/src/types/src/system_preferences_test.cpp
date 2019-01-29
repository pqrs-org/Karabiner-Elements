#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("system_preferences json") {
  {
    krbn::system_preferences p1;
    nlohmann::json j = p1;
    krbn::system_preferences p2 = j;

    REQUIRE(p1 == p2);
    REQUIRE(p1.get_keyboard_fn_state() == false);
    REQUIRE(p1.get_swipe_scroll_direction() == true);
    REQUIRE(p1.get_keyboard_type() == 40);
  }
  {
    krbn::system_preferences p1;
    p1.set_keyboard_fn_state(true);
    p1.set_swipe_scroll_direction(false);
    p1.set_keyboard_type(100);

    nlohmann::json j = p1;
    krbn::system_preferences p2 = j;

    REQUIRE(p1 == p2);
    REQUIRE(p1.get_keyboard_fn_state() == true);
    REQUIRE(p1.get_swipe_scroll_direction() == false);
    REQUIRE(p1.get_keyboard_type() == 100);
  }
}
