#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "device_detail.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("device_identifiers") {
  {
    krbn::device_identifiers di(krbn::vendor_id(1234),
                                krbn::product_id(5678),
                                true,
                                false);
    REQUIRE(di.is_apple() == false);
  }
  {
    krbn::device_identifiers di(krbn::vendor_id(1452),
                                krbn::product_id(610),
                                true,
                                false);
    REQUIRE(di.is_apple() == true);
  }
}

TEST_CASE("input_source_selector") {
  // language
  {
    krbn::input_source_selector selector(std::string("^en$"),
                                         boost::none,
                                         boost::none);

    {
      nlohmann::json expected;
      expected["language"] = "^en$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         boost::none,
                                                         boost::none)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en2"),
                                                         boost::none,
                                                         boost::none)) == false);
  }

  // input_source_id
  {
    krbn::input_source_selector selector(boost::none,
                                         std::string("^com\\.apple\\.keylayout\\.US$"),
                                         boost::none);

    {
      nlohmann::json expected;
      expected["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         std::string("com.apple.keylayout.US"),
                                                         boost::none)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         std::string("com/apple/keylayout/US"),
                                                         boost::none)) == false);
  }

  // input_mode_id
  {
    krbn::input_source_selector selector(boost::none,
                                         boost::none,
                                         std::string("^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$"));

    {
      nlohmann::json expected;
      expected["input_mode_id"] = "^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         std::string("com/apple/inputmethod/Japanese/FullWidthRoman"))) == false);
  }

  // combination
  {
    krbn::input_source_selector selector(std::string("^en$"),
                                         std::string("^com\\.apple\\.keylayout\\.US$"),
                                         std::string("^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$"));

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         boost::none,
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         std::string("com.apple.keylayout.US"),
                                                         boost::none)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // combination
    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com/apple/keylayout/US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == false);
  }

  // none selector
  {
    krbn::input_source_selector selector(boost::none,
                                         boost::none,
                                         boost::none);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         boost::none)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         boost::none,
                                                         boost::none)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         std::string("com.apple.keylayout.US"),
                                                         boost::none)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(boost::none,
                                                         boost::none,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);
  }
}

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

TEST_CASE("make_key_code") {
  REQUIRE(krbn::types::make_key_code("spacebar") == krbn::key_code::spacebar);
  REQUIRE(krbn::types::make_key_code("unknown") == boost::none);
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
    REQUIRE(actual == boost::none);
  }
}

TEST_CASE("make_key_code (modifier_flag)") {
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::zero) == boost::none);
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
  REQUIRE(krbn::types::make_key_code(krbn::modifier_flag::end_) == boost::none);
}

TEST_CASE("make_modifier_flag") {
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::caps_lock) == boost::none);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_control) == krbn::modifier_flag::left_control);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_shift) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_option) == krbn::modifier_flag::left_option);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::left_command) == krbn::modifier_flag::left_command);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_control) == krbn::modifier_flag::right_control);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_shift) == krbn::modifier_flag::right_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_option) == krbn::modifier_flag::right_option);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::right_command) == krbn::modifier_flag::right_command);
  REQUIRE(krbn::types::make_modifier_flag(krbn::key_code::fn) == krbn::modifier_flag::fn);

  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardA)) == boost::none);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardErrorRollOver)) == boost::none);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardLeftShift)) == krbn::modifier_flag::left_shift);
  REQUIRE(krbn::types::make_modifier_flag(krbn::hid_usage_page::button, krbn::hid_usage(1)) == boost::none);
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
    REQUIRE(actual == boost::none);
  }
}

TEST_CASE("make_new_device_id") {
  auto device_id1 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1234},
      {"product_id", 5678},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));

  auto device_id2 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 2345},
      {"product_id", 6789},
      {"is_keyboard", false},
      {"is_pointing_device", true},
  })));

  auto device_id3 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1452},
      {"product_id", 610},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));

  REQUIRE(krbn::types::find_device_detail(device_id1));
  REQUIRE(krbn::types::find_device_detail(device_id1)->get_vendor_id() == krbn::vendor_id(1234));
  REQUIRE(krbn::types::find_device_detail(device_id1)->get_product_id() == krbn::product_id(5678));
  REQUIRE(krbn::types::find_device_detail(device_id1)->get_is_keyboard() == true);
  REQUIRE(krbn::types::find_device_detail(device_id1)->get_is_pointing_device() == false);

  REQUIRE(krbn::types::find_device_detail(device_id2)->get_vendor_id() == krbn::vendor_id(2345));
  REQUIRE(krbn::types::find_device_detail(device_id2)->get_product_id() == krbn::product_id(6789));
  REQUIRE(krbn::types::find_device_detail(device_id2)->get_is_keyboard() == false);
  REQUIRE(krbn::types::find_device_detail(device_id2)->get_is_pointing_device() == true);

  REQUIRE(krbn::types::find_device_detail(krbn::device_id(-1)) == nullptr);

  krbn::types::detach_device_id(device_id1);
  krbn::types::detach_device_id(krbn::device_id(-1));

  REQUIRE(krbn::types::find_device_detail(device_id1) == nullptr);
}
