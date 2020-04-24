#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("make_key_code") {
  REQUIRE(krbn::make_key_code("spacebar") == krbn::key_code::keyboard_spacebar);
  REQUIRE(krbn::make_key_code("left_option") == krbn::key_code::keyboard_left_option);
  REQUIRE(krbn::make_key_code("left_alt") == krbn::key_code::keyboard_left_option);
  REQUIRE(krbn::make_key_code("unknown") == std::nullopt);
  REQUIRE(krbn::make_key_code_name(krbn::key_code::keyboard_spacebar) == std::string("spacebar"));
  REQUIRE(krbn::make_key_code_name(krbn::key_code::keyboard_left_option) == std::string("left_option"));
  REQUIRE(krbn::make_key_code_name(krbn::key_code::keyboard_or_keypad_reserved) == std::string("(number:65535)"));

  REQUIRE(krbn::make_hid_usage_page(krbn::key_code::value_t(1234)) == pqrs::hid::usage_page::keyboard_or_keypad);
  REQUIRE(krbn::make_hid_usage(krbn::key_code::value_t(1234)) == pqrs::hid::usage::value_t(1234));

  {
    auto actual = krbn::make_key_code(pqrs::hid::usage_page::keyboard_or_keypad,
                                      pqrs::hid::usage::keyboard_or_keypad::keyboard_tab);
    REQUIRE(*actual == krbn::key_code::keyboard_tab);
  }
  {
    auto actual = krbn::make_key_code(pqrs::hid::usage_page::apple_vendor_top_case,
                                      pqrs::hid::usage::apple_vendor_top_case::keyboard_fn);
    REQUIRE(*actual == krbn::key_code::apple_vendor_top_case_keyboard_fn);
  }
  {
    auto actual = krbn::make_key_code(pqrs::hid::usage_page::apple_vendor_keyboard,
                                      pqrs::hid::usage::apple_vendor_keyboard::function);
    REQUIRE(*actual == krbn::key_code::apple_vendor_top_case_keyboard_fn);
  }
  {
    auto actual = krbn::make_key_code(pqrs::hid::usage_page::keyboard_or_keypad,
                                      pqrs::hid::usage::value_t(1234));
    REQUIRE(*actual == krbn::key_code::value_t(1234));
  }
  {
    auto actual = krbn::make_key_code(pqrs::hid::usage_page::button,
                                      pqrs::hid::usage::button::button_1);
    REQUIRE(actual == std::nullopt);
  }

  // from_json

  {
    nlohmann::json json("escape");
    REQUIRE(krbn::key_code::value_t(json) == krbn::key_code::keyboard_escape);
  }
  {
    nlohmann::json json(static_cast<uint32_t>(krbn::key_code::keyboard_escape));
    REQUIRE(krbn::key_code::value_t(json) == krbn::key_code::keyboard_escape);
  }
  {
    nlohmann::json json;
    REQUIRE_THROWS_AS(
        krbn::key_code::value_t(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code::value_t(json),
        "json must be string or number, but is `null`");
  }
  {
    nlohmann::json json("unknown_value");
    REQUIRE_THROWS_AS(
        krbn::key_code::value_t(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code::value_t(json),
        "unknown key_code: `\"unknown_value\"`");
  }
}

TEST_CASE("make_key_code (modifier_flag)") {
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::zero) == std::nullopt);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::caps_lock) == krbn::key_code::keyboard_caps_lock);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::left_control) == krbn::key_code::keyboard_left_control);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::left_shift) == krbn::key_code::keyboard_left_shift);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::left_option) == krbn::key_code::keyboard_left_option);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::left_command) == krbn::key_code::keyboard_left_command);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::right_control) == krbn::key_code::keyboard_right_control);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::right_shift) == krbn::key_code::keyboard_right_shift);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::right_option) == krbn::key_code::keyboard_right_option);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::right_command) == krbn::key_code::keyboard_right_command);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::fn) == krbn::key_code::apple_vendor_top_case_keyboard_fn);
  REQUIRE(krbn::make_key_code(krbn::modifier_flag::end_) == std::nullopt);
}

TEST_CASE("make_modifier_flag") {
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_caps_lock) == std::nullopt);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_left_control) == krbn::modifier_flag::left_control);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_left_shift) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_left_option) == krbn::modifier_flag::left_option);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_left_command) == krbn::modifier_flag::left_command);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_right_control) == krbn::modifier_flag::right_control);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_right_shift) == krbn::modifier_flag::right_shift);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_right_option) == krbn::modifier_flag::right_option);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::keyboard_right_command) == krbn::modifier_flag::right_command);
  REQUIRE(krbn::make_modifier_flag(krbn::key_code::apple_vendor_top_case_keyboard_fn) == krbn::modifier_flag::fn);

  REQUIRE(krbn::make_modifier_flag(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_a) == std::nullopt);
  REQUIRE(krbn::make_modifier_flag(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::error_rollover) == std::nullopt);
  REQUIRE(krbn::make_modifier_flag(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::make_modifier_flag(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::button::button_1) == std::nullopt);
}

TEST_CASE("make_consumer_key_code") {
  REQUIRE(krbn::make_consumer_key_code("mute") == krbn::consumer_key_code::mute);
  REQUIRE(!krbn::make_consumer_key_code("unknown"));

  REQUIRE(krbn::make_consumer_key_code_name(krbn::consumer_key_code::mute) == std::string("mute"));
  REQUIRE(krbn::make_consumer_key_code_name(krbn::consumer_key_code::value_t(12345)) == std::string("(number:12345)"));

  REQUIRE(krbn::make_consumer_key_code(pqrs::hid::usage_page::consumer,
                                       pqrs::hid::usage::consumer::mute) == krbn::consumer_key_code::mute);
  REQUIRE(!krbn::make_consumer_key_code(pqrs::hid::usage_page::keyboard_or_keypad,
                                        pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

  REQUIRE(krbn::make_hid_usage_page(krbn::consumer_key_code::mute) == pqrs::hid::usage_page::consumer);
  REQUIRE(krbn::make_hid_usage(krbn::consumer_key_code::mute) == pqrs::hid::usage::consumer::mute);

  // from_json

  {
    nlohmann::json json("mute");
    REQUIRE(krbn::consumer_key_code::value_t(json) == krbn::consumer_key_code::mute);
  }
  {
    nlohmann::json json(static_cast<uint32_t>(krbn::consumer_key_code::mute));
    REQUIRE(krbn::consumer_key_code::value_t(json) == krbn::consumer_key_code::mute);
  }
  {
    nlohmann::json json;
    REQUIRE_THROWS_AS(
        krbn::consumer_key_code::value_t(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code::value_t(json),
        "json must be string or number, but is `null`");
  }
  {
    nlohmann::json json("unknown_value");
    REQUIRE_THROWS_AS(
        krbn::consumer_key_code::value_t(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::consumer_key_code::value_t(json),
        "unknown consumer_key_code: `\"unknown_value\"`");
  }
}

TEST_CASE("make_pointing_button") {
  REQUIRE(krbn::make_pointing_button("button1") == krbn::pointing_button::button1);
  REQUIRE(!krbn::make_pointing_button("unknown"));

  REQUIRE(krbn::make_pointing_button_name(krbn::pointing_button::button1) == std::string("button1"));
  REQUIRE(krbn::make_pointing_button_name(krbn::pointing_button::value_t(12345)) == std::string("(number:12345)"));

  {
    auto actual = krbn::make_pointing_button(pqrs::hid::usage_page::button,
                                             pqrs::hid::usage::button::button_1);
    REQUIRE(*actual == krbn::pointing_button::button1);
  }
  {
    auto actual = krbn::make_pointing_button(pqrs::hid::usage_page::keyboard_or_keypad,
                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_tab);
    REQUIRE(actual == std::nullopt);
  }
}
