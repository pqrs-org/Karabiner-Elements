#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "physical_keyboard_repeat_detector.hpp"
#include "thread_utility.hpp"

TEST_CASE("is_repeating") {
  krbn::physical_keyboard_repeat_detector physical_keyboard_repeat_detector;
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == false);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")), krbn::event_type::key_up);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == false);

  // ----------------------------------------
  // Cancel by modifier key

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")), krbn::event_type::key_down);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == true);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("left_shift")), krbn::event_type::key_down);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == false);

  // ----------------------------------------
  // Cancel by key_up

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")), krbn::event_type::key_down);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == true);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("escape")), krbn::event_type::key_down);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == true);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("left_shift")), krbn::event_type::key_up);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == true);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("spacebar")), krbn::event_type::key_up);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == true);

  physical_keyboard_repeat_detector.set(krbn::device_id(1), *(krbn::types::get_key_code("escape")), krbn::event_type::key_up);
  REQUIRE(physical_keyboard_repeat_detector.is_repeating(krbn::device_id(1)) == false);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
