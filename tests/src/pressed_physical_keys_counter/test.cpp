#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "pressed_physical_keys_counter.hpp"
#include "thread_utility.hpp"

TEST_CASE("pressed_physical_keys_counter") {
  krbn::pressed_physical_keys_counter pressed_physical_keys_counter;

  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);

  // ----------------------------------------

  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(2)) == true);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);

  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), krbn::pointing_button::button4);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == true);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), krbn::pointing_button::button4);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);

  // ----------------------------------------
  // Push twice

  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);

  // ----------------------------------------
  // Mixed values

  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("tab")));
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), krbn::pointing_button::button1);
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), krbn::pointing_button::button2);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), *(krbn::types::get_key_code("tab")));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), krbn::pointing_button::button1);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1), krbn::pointing_button::button2);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);

  // ----------------------------------------
  // Erase by device_id

  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")));
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), *(krbn::types::get_key_code("tab")));
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), krbn::pointing_button::button1);
  pressed_physical_keys_counter.emplace_back_pressed_key(krbn::device_id(1), krbn::pointing_button::button2);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);

  pressed_physical_keys_counter.erase_all_pressed_keys(krbn::device_id(1));
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);
}

TEST_CASE("pressed_physical_keys_counter.usage_page") {
  krbn::pressed_physical_keys_counter pressed_physical_keys_counter;

  REQUIRE(pressed_physical_keys_counter.update(krbn::device_id(1),
                                               krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                               krbn::hid_usage(kHIDUsage_KeyboardErrorRollOver),
                                               1) == false);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);

  REQUIRE(pressed_physical_keys_counter.update(krbn::device_id(1),
                                               krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                               krbn::hid_usage(kHIDUsage_KeyboardTab),
                                               1) == true);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);

  REQUIRE(pressed_physical_keys_counter.update(krbn::device_id(1),
                                               krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                               krbn::hid_usage(kHIDUsage_KeyboardTab),
                                               0) == true);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);

  REQUIRE(pressed_physical_keys_counter.update(krbn::device_id(1),
                                               krbn::hid_usage_page(kHIDPage_Button),
                                               krbn::hid_usage(1),
                                               1) == true);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == false);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == true);

  REQUIRE(pressed_physical_keys_counter.update(krbn::device_id(1),
                                               krbn::hid_usage_page(kHIDPage_Button),
                                               krbn::hid_usage(1),
                                               0) == true);
  REQUIRE(pressed_physical_keys_counter.empty(krbn::device_id(1)) == true);
  REQUIRE(pressed_physical_keys_counter.is_pointing_button_pressed(krbn::device_id(1)) == false);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
