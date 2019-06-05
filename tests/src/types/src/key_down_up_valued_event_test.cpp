#include <catch2/catch.hpp>

#include "types.hpp"
#include <set>

TEST_CASE("key_down_up_valued_event") {
  {
    krbn::key_down_up_valued_event e(krbn::key_code::a);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(nlohmann::json(e).get<krbn::key_down_up_valued_event>() == e);
  }
  {
    krbn::key_down_up_valued_event e(krbn::key_code::left_shift);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(nlohmann::json(e).get<krbn::key_down_up_valued_event>() == e);
  }
  {
    krbn::key_down_up_valued_event e(krbn::consumer_key_code::mute);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(nlohmann::json(e).get<krbn::key_down_up_valued_event>() == e);
  }
  {
    krbn::key_down_up_valued_event e(krbn::pointing_button::button1);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(nlohmann::json(e).get<krbn::key_down_up_valued_event>() == e);
  }
  {
    std::set<krbn::key_down_up_valued_event> map;
    map.insert(krbn::key_down_up_valued_event(krbn::consumer_key_code::mute));
    map.insert(krbn::key_down_up_valued_event(krbn::key_code::b));
    map.insert(krbn::key_down_up_valued_event(krbn::key_code::a));
    map.insert(krbn::key_down_up_valued_event(krbn::key_code::c));
    map.insert(krbn::key_down_up_valued_event(krbn::pointing_button::button2));
    map.insert(krbn::key_down_up_valued_event(krbn::pointing_button::button1));

    int i = 0;
    for (const auto& m : map) {
      switch (i++) {
        case 0:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::key_code::a));
          break;
        case 1:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::key_code::b));
          break;
        case 2:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::key_code::c));
          break;
        case 3:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::consumer_key_code::mute));
          break;
        case 4:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::pointing_button::button1));
          break;
        case 5:
          REQUIRE(m == krbn::key_down_up_valued_event(krbn::pointing_button::button2));
          break;
      }
    }
  }
}
