#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "keyboard_repeat_detector.hpp"

TEST_CASE("is_repeating") {
  krbn::keyboard_repeat_detector keyboard_repeat_detector;
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                               *(krbn::types::make_hid_usage(krbn::key_code::spacebar)),
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // Ignore modifier keys

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                               *(krbn::types::make_hid_usage(krbn::key_code::spacebar)),
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::left_shift)),
                               *(krbn::types::make_hid_usage(krbn::key_code::left_shift)),
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  // ----------------------------------------
  // Cancel by key_up

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                               *(krbn::types::make_hid_usage(krbn::key_code::spacebar)),
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::escape)),
                               *(krbn::types::make_hid_usage(krbn::key_code::escape)),
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::left_shift)),
                               *(krbn::types::make_hid_usage(krbn::key_code::left_shift)),
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                               *(krbn::types::make_hid_usage(krbn::key_code::spacebar)),
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(*(krbn::types::make_hid_usage_page(krbn::key_code::escape)),
                               *(krbn::types::make_hid_usage(krbn::key_code::escape)),
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // hid_value

  {
    krbn::hid_value hid_value(krbn::absolute_time_point(0),
                              1,
                              *(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                              *(krbn::types::make_hid_usage(krbn::key_code::spacebar)));
    keyboard_repeat_detector.set(hid_value);
    REQUIRE(keyboard_repeat_detector.is_repeating() == true);
  }
  {
    krbn::hid_value hid_value(krbn::absolute_time_point(0),
                              0,
                              *(krbn::types::make_hid_usage_page(krbn::key_code::spacebar)),
                              *(krbn::types::make_hid_usage(krbn::key_code::spacebar)));
    keyboard_repeat_detector.set(hid_value);
    REQUIRE(keyboard_repeat_detector.is_repeating() == false);
  }
}
