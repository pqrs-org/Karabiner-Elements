#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("make_key_code") {
  REQUIRE(krbn::types::make_key_code("spacebar") == krbn::key_code::spacebar);
  REQUIRE(krbn::types::make_key_code("unknown") == std::nullopt);
  REQUIRE(krbn::types::make_key_code_name(krbn::key_code::spacebar) == std::string("spacebar"));
  REQUIRE(krbn::types::make_key_code_name(krbn::key_code::left_option) == std::string("left_alt"));
  REQUIRE(krbn::types::make_key_code_name(krbn::key_code::extra_) == std::string("(number:65536)"));

  REQUIRE(krbn::types::make_hid_usage_page(krbn::key_code(1234)) == krbn::hid_usage_page::keyboard_or_keypad);
  REQUIRE(krbn::types::make_hid_usage(krbn::key_code(1234)) == krbn::hid_usage(1234));

  {
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                             krbn::hid_usage(kHIDUsage_KeyboardTab));
    REQUIRE(*actual == krbn::key_code(kHIDUsage_KeyboardTab));
  }
  {
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorTopCase),
                                             krbn::hid_usage(krbn::kHIDUsage_AV_TopCase_KeyboardFn));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorKeyboard),
                                             krbn::hid_usage(krbn::kHIDUsage_AppleVendorKeyboard_Function));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page::keyboard_or_keypad,
                                             krbn::hid_usage(1234));
    REQUIRE(*actual == krbn::key_code(1234));
  }
  {
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page(kHIDPage_Button),
                                             krbn::hid_usage(1));
    REQUIRE(actual == std::nullopt);
  }

  // from_json

  {
    nlohmann::json json("escape");
    REQUIRE(krbn::key_code(json) == krbn::key_code::escape);
  }
  {
    nlohmann::json json(static_cast<uint32_t>(krbn::key_code::escape));
    REQUIRE(krbn::key_code(json) == krbn::key_code::escape);
  }
  {
    nlohmann::json json;
    REQUIRE_THROWS_AS(
        krbn::key_code(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code(json),
        "json must be string or number, but is `null`");
  }
  {
    nlohmann::json json("unknown_value");
    REQUIRE_THROWS_AS(
        krbn::key_code(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code(json),
        "unknown key_code: `\"unknown_value\"`");
  }
}

TEST_CASE("make_key_code (modifier_flag)") {
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::zero) == std::nullopt);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::caps_lock) == krbn::key_code::caps_lock);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::left_control) == krbn::key_code::left_control);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::left_shift) == krbn::key_code::left_shift);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::left_option) == krbn::key_code::left_option);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::left_command) == krbn::key_code::left_command);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::right_control) == krbn::key_code::right_control);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::right_shift) == krbn::key_code::right_shift);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::right_option) == krbn::key_code::right_option);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::right_command) == krbn::key_code::right_command);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::fn) == krbn::key_code::fn);
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::end_) == std::nullopt);
}

TEST_CASE("make_modifier_flag") {
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::caps_lock) == std::nullopt);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_control) == krbn::modifier_flag::left_control);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_shift) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_option) == krbn::modifier_flag::left_option);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_command) == krbn::modifier_flag::left_command);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_control) == krbn::modifier_flag::right_control);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_shift) == krbn::modifier_flag::right_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_option) == krbn::modifier_flag::right_option);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_command) == krbn::modifier_flag::right_command);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::fn) == krbn::modifier_flag::fn);

  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardA)) == std::nullopt);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardErrorRollOver)) == std::nullopt);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardLeftShift)) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::button, krbn::hid_usage(1)) == std::nullopt);
}

TEST_CASE("make_consumer_key_code") {
  REQUIRE(krbn::types::make_consumer_key_code("mute") == krbn::consumer_key_code::mute);
  REQUIRE(!krbn::types::make_consumer_key_code("unknown"));

  REQUIRE(krbn::types::make_consumer_key_code_name(krbn::consumer_key_code::mute) == std::string("mute"));
  REQUIRE(krbn::types::make_consumer_key_code_name(krbn::consumer_key_code(12345)) == std::string("(number:12345)"));

  REQUIRE(krbn::types::make_consumer_key_code(krbn::hid_usage_page::consumer, krbn::hid_usage::csmr_mute) == krbn::consumer_key_code::mute);
  REQUIRE(!krbn::types::make_consumer_key_code(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardA)));

  REQUIRE(krbn::types::make_hid_usage_page(krbn::consumer_key_code::mute) == krbn::hid_usage_page::consumer);
  REQUIRE(krbn::types::make_hid_usage(krbn::consumer_key_code::mute) == krbn::hid_usage::csmr_mute);

  // from_json

  {
    nlohmann::json json("mute");
    REQUIRE(krbn::consumer_key_code(json) == krbn::consumer_key_code::mute);
  }
  {
    nlohmann::json json(static_cast<uint32_t>(krbn::consumer_key_code::mute));
    REQUIRE(krbn::consumer_key_code(json) == krbn::consumer_key_code::mute);
  }
  {
    nlohmann::json json;
    REQUIRE_THROWS_AS(
        krbn::consumer_key_code(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::key_code(json),
        "json must be string or number, but is `null`");
  }
  {
    nlohmann::json json("unknown_value");
    REQUIRE_THROWS_AS(
        krbn::consumer_key_code(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::consumer_key_code(json),
        "unknown consumer_key_code: `\"unknown_value\"`");
  }
}

TEST_CASE("make_pointing_button") {
  REQUIRE(krbn::types::make_pointing_button("button1") == krbn::pointing_button::button1);
  REQUIRE(!krbn::types::make_pointing_button("unknown"));

  REQUIRE(krbn::types::make_pointing_button_name(krbn::pointing_button::button1) == std::string("button1"));
  REQUIRE(krbn::types::make_pointing_button_name(krbn::pointing_button(12345)) == std::string("(number:12345)"));

  {
    auto actual = krbn::types::make_pointing_button(krbn::hid_usage_page(kHIDPage_Button),
                                                    krbn::hid_usage(1));
    REQUIRE(*actual == krbn::pointing_button::button1);
  }
  {
    auto actual = krbn::types::make_pointing_button(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                                    krbn::hid_usage(kHIDUsage_KeyboardTab));
    REQUIRE(actual == std::nullopt);
  }
}
