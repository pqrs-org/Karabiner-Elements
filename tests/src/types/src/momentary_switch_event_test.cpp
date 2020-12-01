#include <catch2/catch.hpp>

#include "types.hpp"
#include <set>

TEST_CASE("momentary_switch_event") {
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::left_shift);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::consumer::mute);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::apple_vendor_keyboard::expose_all);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::apple_vendor_keyboard::function);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::fn);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                   pqrs::hid::usage::apple_vendor_top_case::brightness_down);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                   pqrs::hid::usage::apple_vendor_top_case::keyboard_fn);
    REQUIRE(e.make_modifier_flag() == krbn::modifier_flag::fn);
    REQUIRE(e.modifier_flag() == true);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::button::button_1);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == true);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }
  {
    std::set<krbn::momentary_switch_event> map;
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                            pqrs::hid::usage::consumer::mute));
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                            pqrs::hid::usage::button::button_2));
    map.insert(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                            pqrs::hid::usage::button::button_1));

    int i = 0;
    for (const auto& m : map) {
      switch (i++) {
        case 0:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
          break;
        case 1:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
          break;
        case 2:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
          break;
        case 3:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                    pqrs::hid::usage::consumer::mute));
          break;
        case 4:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                    pqrs::hid::usage::button::button_1));
          break;
        case 5:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                    pqrs::hid::usage::button::button_2));
          break;
      }
    }
  }
}

TEST_CASE("momentary_switch_event json") {
  {
    std::string expected("{\"pointing_button\":\"button1\"}");
    nlohmann::json actual = krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                         pqrs::hid::usage::button::button_1);
    REQUIRE(actual.dump() == expected);
  }
  {
    std::string expected("{\"pointing_button\":\"(number:1234)\"}");
    nlohmann::json actual = krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                         pqrs::hid::usage::value_t(1234));
    REQUIRE(actual.dump() == expected);
  }
}
