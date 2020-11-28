#include <catch2/catch.hpp>

#include "types.hpp"
#include <set>

TEST_CASE("momentary_switch_event") {
  {
    krbn::momentary_switch_event e(krbn::key_code::keyboard_a);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::key_code::keyboard_left_shift);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::left_shift);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::consumer_key_code::mute);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::apple_vendor_keyboard_key_code::expose_all);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::apple_vendor_keyboard_key_code::function);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::fn);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::apple_vendor_top_case_key_code::brightness_down);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::apple_vendor_top_case_key_code::keyboard_fn);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::fn);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(krbn::pointing_button::button1);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == true);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    std::set<krbn::momentary_switch_event> map;
    map.insert(krbn::momentary_switch_event(krbn::consumer_key_code::mute));
    map.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_b));
    map.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_a));
    map.insert(krbn::momentary_switch_event(krbn::key_code::keyboard_c));
    map.insert(krbn::momentary_switch_event(krbn::pointing_button::button2));
    map.insert(krbn::momentary_switch_event(krbn::pointing_button::button1));

    int i = 0;
    for (const auto& m : map) {
      switch (i++) {
        case 0:
          REQUIRE(m == krbn::momentary_switch_event(krbn::key_code::keyboard_a));
          break;
        case 1:
          REQUIRE(m == krbn::momentary_switch_event(krbn::key_code::keyboard_b));
          break;
        case 2:
          REQUIRE(m == krbn::momentary_switch_event(krbn::key_code::keyboard_c));
          break;
        case 3:
          REQUIRE(m == krbn::momentary_switch_event(krbn::consumer_key_code::mute));
          break;
        case 4:
          REQUIRE(m == krbn::momentary_switch_event(krbn::pointing_button::button1));
          break;
        case 5:
          REQUIRE(m == krbn::momentary_switch_event(krbn::pointing_button::button2));
          break;
      }
    }
  }
}

TEST_CASE("momentary_switch_event json") {
  {
    std::string expected("{\"pointing_button\":\"button1\"}");
    nlohmann::json actual = krbn::momentary_switch_event(pqrs::hid::usage_page::button, pqrs::hid::usage::button::button_1);
    REQUIRE(actual.dump() == expected);
  }
  {
    std::string expected("{\"pointing_button\":\"(number:1234)\"}");
    nlohmann::json actual = krbn::momentary_switch_event(pqrs::hid::usage_page::button, pqrs::hid::usage::value_t(1234));
    REQUIRE(actual.dump() == expected);
  }
}
