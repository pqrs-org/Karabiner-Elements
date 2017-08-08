#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"
#include "types.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("get_key_code") {
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                            krbn::hid_usage(kHIDUsage_KeyboardTab));
    REQUIRE(*actual == krbn::key_code(kHIDUsage_KeyboardTab));
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorTopCase),
                                            krbn::hid_usage(krbn::kHIDUsage_AV_TopCase_KeyboardFn));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorKeyboard),
                                            krbn::hid_usage(krbn::kHIDUsage_AppleVendorKeyboard_Function));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(kHIDPage_Button),
                                            krbn::hid_usage(1));
    REQUIRE(actual == boost::none);
  }
}

TEST_CASE("get_key_code (modifier_flag)") {
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::zero) == boost::none);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::caps_lock) == krbn::key_code::caps_lock);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::left_control) == krbn::key_code::left_control);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::left_shift) == krbn::key_code::left_shift);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::left_option) == krbn::key_code::left_option);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::left_command) == krbn::key_code::left_command);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::right_control) == krbn::key_code::right_control);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::right_shift) == krbn::key_code::right_shift);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::right_option) == krbn::key_code::right_option);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::right_command) == krbn::key_code::right_command);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::fn) == krbn::key_code::fn);
  REQUIRE(krbn::types::get_key_code(krbn::modifier_flag::end_) == boost::none);
}

TEST_CASE("get_modifier_flag") {
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::caps_lock) == krbn::modifier_flag::zero);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::left_control) == krbn::modifier_flag::left_control);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::left_shift) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::left_option) == krbn::modifier_flag::left_option);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::left_command) == krbn::modifier_flag::left_command);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::right_control) == krbn::modifier_flag::right_control);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::right_shift) == krbn::modifier_flag::right_shift);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::right_option) == krbn::modifier_flag::right_option);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::right_command) == krbn::modifier_flag::right_command);
  REQUIRE(krbn::types::get_modifier_flag(krbn::key_code::fn) == krbn::modifier_flag::fn);
}

TEST_CASE("get_pointing_button") {
  {
    auto actual = krbn::types::get_pointing_button(krbn::hid_usage_page(kHIDPage_Button),
                                                   krbn::hid_usage(1));
    REQUIRE(*actual == krbn::pointing_button::button1);
  }
  {
    auto actual = krbn::types::get_pointing_button(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                                   krbn::hid_usage(kHIDUsage_KeyboardTab));
    REQUIRE(actual == boost::none);
  }
}

TEST_CASE("make_new_device_id") {
  auto device_id1 = krbn::types::make_new_device_id(krbn::vendor_id(1234),
                                                    krbn::product_id(5678),
                                                    true,
                                                    false);
  auto device_id2 = krbn::types::make_new_device_id(krbn::vendor_id(2345),
                                                    krbn::product_id(6789),
                                                    false,
                                                    true);

  REQUIRE(krbn::types::find_device_identifier(device_id1)->get_vendor_id() == krbn::vendor_id(1234));
  REQUIRE(krbn::types::find_device_identifier(device_id1)->get_product_id() == krbn::product_id(5678));
  REQUIRE(krbn::types::find_device_identifier(device_id1)->get_is_keyboard() == true);
  REQUIRE(krbn::types::find_device_identifier(device_id1)->get_is_pointing_device() == false);

  REQUIRE(krbn::types::find_device_identifier(device_id2)->get_vendor_id() == krbn::vendor_id(2345));
  REQUIRE(krbn::types::find_device_identifier(device_id2)->get_product_id() == krbn::product_id(6789));
  REQUIRE(krbn::types::find_device_identifier(device_id2)->get_is_keyboard() == false);
  REQUIRE(krbn::types::find_device_identifier(device_id2)->get_is_pointing_device() == true);

  REQUIRE(krbn::types::find_device_identifier(krbn::device_id(-1)) == nullptr);

  krbn::types::detach_device_id(device_id1);
  krbn::types::detach_device_id(krbn::device_id(-1));

  REQUIRE(krbn::types::find_device_identifier(device_id1) == nullptr);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
