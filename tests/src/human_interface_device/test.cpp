#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "human_interface_device.hpp"
#include "thread_utility.hpp"

TEST_CASE("keyboard_repeat_detector.is_repeating") {
  krbn::human_interface_device::keyboard_repeat_detector keyboard_repeat_detector;
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  keyboard_repeat_detector.set(krbn::key_code::spacebar, krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // Cancel by modifier key

  keyboard_repeat_detector.set(krbn::key_code::spacebar, krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(krbn::key_code::left_shift, krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // Cancel by key_up

  keyboard_repeat_detector.set(krbn::key_code::spacebar, krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(krbn::key_code::escape, krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(krbn::key_code::left_shift, krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(krbn::key_code::spacebar, krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(krbn::key_code::escape, krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
