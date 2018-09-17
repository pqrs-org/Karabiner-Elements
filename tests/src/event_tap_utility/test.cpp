#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "event_tap_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("make_event") {
  {
    auto actual = krbn::event_tap_utility::make_event(kCGEventLeftMouseDown, nullptr);
    REQUIRE(actual->first == krbn::event_type::key_down);
    REQUIRE(actual->second == krbn::event_queue::event(krbn::pointing_button::button1));
  }
  {
    auto actual = krbn::event_tap_utility::make_event(kCGEventOtherMouseUp, nullptr);
    REQUIRE(actual->first == krbn::event_type::key_up);
    REQUIRE(actual->second == krbn::event_queue::event(krbn::pointing_button::button3));
  }
}
