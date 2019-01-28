#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("sizeof") {
  REQUIRE(sizeof(krbn::vendor_id) == 4);
  REQUIRE(sizeof(krbn::product_id) == 4);
  REQUIRE(sizeof(krbn::location_id) == 4);
}

TEST_CASE("operation_type") {
  krbn::operation_type_connect_console_user_server_struct s;
  std::vector<uint8_t> buffer(sizeof(s));
  memcpy(&(buffer[0]), &s, sizeof(s));
  REQUIRE(krbn::types::find_operation_type(buffer) == krbn::operation_type::connect_console_user_server);
}

TEST_CASE("grabbable_state") {
  {
    krbn::grabbable_state grabbable_state;
    REQUIRE(grabbable_state.get_device_id() == krbn::device_id(0));
    REQUIRE(grabbable_state.get_state() == krbn::grabbable_state::state::grabbable);
    REQUIRE(grabbable_state.get_ungrabbable_temporarily_reason() == krbn::grabbable_state::ungrabbable_temporarily_reason::none);
    REQUIRE(grabbable_state.get_time_stamp() == krbn::absolute_time_point(0));
  }
  {
    krbn::grabbable_state grabbable_state(krbn::device_id(1234),
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                          krbn::absolute_time_point(1000));
    REQUIRE(grabbable_state.get_device_id() == krbn::device_id(1234));
    REQUIRE(grabbable_state.get_state() == krbn::grabbable_state::state::ungrabbable_temporarily);
    REQUIRE(grabbable_state.get_ungrabbable_temporarily_reason() == krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating);
    REQUIRE(grabbable_state.get_time_stamp() == krbn::absolute_time_point(1000));
  }
}

TEST_CASE("grabbable_state::equals_except_time_stamp") {
  {
    krbn::grabbable_state grabbable_state1(krbn::device_id(1234),
                                           krbn::grabbable_state::state::ungrabbable_temporarily,
                                           krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                           krbn::absolute_time_point(1000));
    krbn::grabbable_state grabbable_state2(krbn::device_id(1234),
                                           krbn::grabbable_state::state::ungrabbable_temporarily,
                                           krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                           krbn::absolute_time_point(2000));
    krbn::grabbable_state grabbable_state3(krbn::device_id(1234),
                                           krbn::grabbable_state::state::device_error,
                                           krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                           krbn::absolute_time_point(3000));
    REQUIRE(grabbable_state1 != grabbable_state2);
    REQUIRE(grabbable_state1.equals_except_time_stamp(grabbable_state2));
    REQUIRE(!grabbable_state1.equals_except_time_stamp(grabbable_state3));
  }
}

TEST_CASE("make_key_code") {
  REQUIRE(krbn::types::make_key_code("spacebar") == krbn::key_code::spacebar);
  REQUIRE(krbn::types::make_key_code("unknown") == std::nullopt);
  REQUIRE(krbn::types::make_key_code_name(krbn::key_code::spacebar) == std::string("spacebar"));
  REQUIRE(krbn::types::make_key_code_name(krbn::key_code::left_option) == std::string("left_alt"));

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
    auto actual = krbn::types::make_key_code(krbn::hid_usage_page(kHIDPage_Button),
                                             krbn::hid_usage(1));
    REQUIRE(actual == std::nullopt);
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

  REQUIRE(krbn::types::make_consumer_key_code(krbn::hid_usage_page::consumer, krbn::hid_usage::csmr_mute) == krbn::consumer_key_code::mute);
  REQUIRE(!krbn::types::make_consumer_key_code(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardA)));

  REQUIRE(krbn::types::make_hid_usage_page(krbn::consumer_key_code::mute) == krbn::hid_usage_page::consumer);
  REQUIRE(krbn::types::make_hid_usage(krbn::consumer_key_code::mute) == krbn::hid_usage::csmr_mute);
}

TEST_CASE("make_pointing_button") {
  REQUIRE(krbn::types::make_pointing_button("button1") == krbn::pointing_button::button1);
  REQUIRE(!krbn::types::make_pointing_button("unknown"));

  REQUIRE(krbn::types::make_pointing_button_name(krbn::pointing_button::button1) == std::string("button1"));

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
