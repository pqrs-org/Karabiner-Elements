#include <catch2/catch.hpp>

#include "types.hpp"
#include <set>

TEST_CASE("momentary_switch_event") {
  //
  // usage_page::keyboard_or_keypad
  //

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

  //
  // usage_page::consumer
  //

  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::consumer::mute);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == false);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }

  //
  // usage_page::apple_vendor_keyboard
  //

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
    // Not target usage

    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::apple_vendor_keyboard::caps_lock_delay_enable);
    REQUIRE(e.make_usage_pair() == std::nullopt);
  }

  //
  // usage_page::apple_vendor_top_case
  //

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

  //
  // usage_page::button
  //

  {
    krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::button::button_1);
    REQUIRE(e.make_modifier_flag() == std::nullopt);
    REQUIRE(e.modifier_flag() == false);
    REQUIRE(e.pointing_button() == true);
    REQUIRE(nlohmann::json(e).get<krbn::momentary_switch_event>() == e);
  }

  //
  // map
  //

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
      std::cout << nlohmann::json(m).dump() << std::endl;

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
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                    pqrs::hid::usage::button::button_1));
          break;
        case 4:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                    pqrs::hid::usage::button::button_2));
          break;
        case 5:
          REQUIRE(m == krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                    pqrs::hid::usage::consumer::mute));
          break;
      }
    }
  }
}

TEST_CASE("momentary_switch_event json") {
  //
  // key_code
  //

  {
    std::string expected("{\"key_code\":\"escape\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_escape);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }
  {
    // Alias
    std::string expected("{\"key_code\":\"left_option\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }
  {
    // Other usage_page
    krbn::momentary_switch_event expected(pqrs::hid::usage_page::apple_vendor_keyboard,
                                          pqrs::hid::usage::apple_vendor_keyboard::expose_all);
    auto json = nlohmann::json::object({{"key_code", "mission_control"}});
    REQUIRE(json.get<krbn::momentary_switch_event>() == expected);
  }

  //
  // consumer_key_code
  //

  {
    std::string expected("{\"consumer_key_code\":\"mute\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::consumer::mute);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }

  //
  // apple_vendor_keyboard_key_code
  //

  {
    std::string expected("{\"apple_vendor_keyboard_key_code\":\"spotlight\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::apple_vendor_keyboard::spotlight);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }

  //
  // apple_vendor_top_case_key_code
  //

  {
    std::string expected("{\"apple_vendor_top_case_key_code\":\"brightness_down\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::apple_vendor_top_case,
                                   pqrs::hid::usage::apple_vendor_top_case::brightness_down);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }

  //
  // pointing_button
  //

  {
    std::string expected("{\"pointing_button\":\"button1\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::button::button_1);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }

  //
  // Unnamed
  //

  {
    std::string expected("{\"pointing_button\":\"(number:1234)\"}");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::value_t(1234));
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
    REQUIRE(actual.get<krbn::momentary_switch_event>() == e);
  }

  //
  // Number
  //

  {
    krbn::momentary_switch_event expected(pqrs::hid::usage_page::keyboard_or_keypad,
                                          pqrs::hid::usage::value_t(1234));
    auto json = nlohmann::json::object({{"key_code", 1234}});
    REQUIRE(json.get<krbn::momentary_switch_event>() == expected);
  }

  //
  // Not target
  //

  {

    std::string expected("null");
    krbn::momentary_switch_event e(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::consumer::ac_pan);
    nlohmann::json actual = e;
    REQUIRE(actual.dump() == expected);
  }

  //
  // Errors
  //

  {
    nlohmann::json json;
    REQUIRE_THROWS_AS(
        krbn::momentary_switch_event(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::momentary_switch_event(json),
        "json must be object, but is `null`");
  }
  {
    auto json = nlohmann::json::object({{"key_code", "unknown_value"}});
    REQUIRE_THROWS_AS(
        krbn::momentary_switch_event(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::momentary_switch_event(json),
        "unknown key_code: `\"unknown_value\"`");
  }
}

TEST_CASE("momentary_switch_event(modifier_flag)") {
  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::zero).make_usage_pair() == std::nullopt);

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::caps_lock).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::left_control).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::left_shift).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::left_option).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::left_command).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::right_control).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::right_shift).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::right_option).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::right_command).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::fn).make_usage_pair() ==
          pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case,
                                pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));

  REQUIRE(krbn::momentary_switch_event(krbn::modifier_flag::end_).make_usage_pair() == std::nullopt);
}

TEST_CASE("momentary_switch_event::make_modifier_flag") {
  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_a)
              .make_modifier_flag() == std::nullopt);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock)
              .make_modifier_flag() == std::nullopt);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control)
              .make_modifier_flag() == krbn::modifier_flag::left_control);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift)
              .make_modifier_flag() == krbn::modifier_flag::left_shift);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt)
              .make_modifier_flag() == krbn::modifier_flag::left_option);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui)
              .make_modifier_flag() == krbn::modifier_flag::left_command);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control)
              .make_modifier_flag() == krbn::modifier_flag::right_control);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift)
              .make_modifier_flag() == krbn::modifier_flag::right_shift);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt)
              .make_modifier_flag() == krbn::modifier_flag::right_option);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui)
              .make_modifier_flag() == krbn::modifier_flag::right_command);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                       pqrs::hid::usage::apple_vendor_keyboard::function)
              .make_modifier_flag() == krbn::modifier_flag::fn);

  REQUIRE(krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                       pqrs::hid::usage::apple_vendor_top_case::keyboard_fn)
              .make_modifier_flag() == krbn::modifier_flag::fn);
}
