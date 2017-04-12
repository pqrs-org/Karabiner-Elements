#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/event_queue.hpp"
#include "thread_utility.hpp"

TEST_CASE("push_back_event") {
  // normal order
  {
    krbn::manipulator::event_queue event_queue;

    event_queue.push_back_event(100,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(200,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(300,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(400,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    std::vector<krbn::manipulator::event_queue::queued_event> expected({
        krbn::manipulator::event_queue::queued_event(100,
                                                     *(krbn::types::get_key_code("a")),
                                                     krbn::event_type::key_down),
        krbn::manipulator::event_queue::queued_event(200,
                                                     *(krbn::types::get_key_code("left_shift")),
                                                     krbn::event_type::key_down),
        krbn::manipulator::event_queue::queued_event(300,
                                                     *(krbn::types::get_key_code("left_shift")),
                                                     krbn::event_type::key_up),
        krbn::manipulator::event_queue::queued_event(400,
                                                     *(krbn::types::get_key_code("a")),
                                                     krbn::event_type::key_up),
    });
    REQUIRE(event_queue.get_events() == expected);

    REQUIRE(event_queue.get_events()[0].get_valid() == true);
    REQUIRE(event_queue.get_events()[0].get_lazy() == false);
  }

  // reorder events
  {
    krbn::manipulator::event_queue event_queue;

    // push "a" and "left_shift" (key_down) at the same time.
    event_queue.push_back_event(100,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(100,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(200,
                                *(krbn::types::get_key_code("spacebar")),
                                krbn::event_type::key_down);
    // push "a" and "left_shift" (key_up) at the same time.
    event_queue.push_back_event(300,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(300,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    std::vector<krbn::manipulator::event_queue::queued_event> expected({
        krbn::manipulator::event_queue::queued_event(100,
                                                     *(krbn::types::get_key_code("left_shift")),
                                                     krbn::event_type::key_down),
        krbn::manipulator::event_queue::queued_event(100,
                                                     *(krbn::types::get_key_code("a")),
                                                     krbn::event_type::key_down),
        krbn::manipulator::event_queue::queued_event(200,
                                                     *(krbn::types::get_key_code("spacebar")),
                                                     krbn::event_type::key_down),
        krbn::manipulator::event_queue::queued_event(300,
                                                     *(krbn::types::get_key_code("a")),
                                                     krbn::event_type::key_up),
        krbn::manipulator::event_queue::queued_event(300,
                                                     *(krbn::types::get_key_code("left_shift")),
                                                     krbn::event_type::key_up),

    });
    REQUIRE(event_queue.get_events() == expected);
  }
}

TEST_CASE("compare") {
  krbn::manipulator::event_queue::queued_event spacebar_down(100,
                                                             *(krbn::types::get_key_code("spacebar")),
                                                             krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event right_shift_down(100,
                                                                *(krbn::types::get_key_code("right_shift")),
                                                                krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event escape_down(200,
                                                           *(krbn::types::get_key_code("escape")),
                                                           krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event spacebar_up(300,
                                                           *(krbn::types::get_key_code("spacebar")),
                                                           krbn::event_type::key_up);
  krbn::manipulator::event_queue::queued_event right_shift_up(300,
                                                              *(krbn::types::get_key_code("right_shift")),
                                                              krbn::event_type::key_up);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, escape_down) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(escape_down, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, right_shift_down) == false);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_down, spacebar_down) == true);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, right_shift_up) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_up, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_up, right_shift_up) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_up, spacebar_up) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_up, right_shift_down) == false);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_down, spacebar_up) == true);
}

TEST_CASE("push_back_event.usage_page") {
  krbn::manipulator::event_queue event_queue;

  event_queue.push_back_event(100,
                              kHIDPage_KeyboardOrKeypad,
                              kHIDUsage_KeyboardTab,
                              1);
  event_queue.push_back_event(200,
                              kHIDPage_KeyboardOrKeypad,
                              kHIDUsage_KeyboardTab,
                              0);
  event_queue.push_back_event(300,
                              kHIDPage_Button,
                              2,
                              1);
  event_queue.push_back_event(400,
                              kHIDPage_Button,
                              2,
                              0);
  event_queue.push_back_event(500,
                              kHIDPage_GenericDesktop,
                              kHIDUsage_GD_X,
                              10);
  event_queue.push_back_event(600,
                              kHIDPage_GenericDesktop,
                              kHIDUsage_GD_Y,
                              -10);

  std::vector<krbn::manipulator::event_queue::queued_event> expected({
      krbn::manipulator::event_queue::queued_event(100,
                                                   *(krbn::types::get_key_code("tab")),
                                                   krbn::event_type::key_down),
      krbn::manipulator::event_queue::queued_event(200,
                                                   *(krbn::types::get_key_code("tab")),
                                                   krbn::event_type::key_up),
      krbn::manipulator::event_queue::queued_event(300,
                                                   krbn::pointing_button::button2,
                                                   krbn::event_type::key_down),
      krbn::manipulator::event_queue::queued_event(400,
                                                   krbn::pointing_button::button2,
                                                   krbn::event_type::key_up),
      krbn::manipulator::event_queue::queued_event(500,
                                                   krbn::manipulator::event_queue::queued_event::type::pointing_x,
                                                   10),
      krbn::manipulator::event_queue::queued_event(600,
                                                   krbn::manipulator::event_queue::queued_event::type::pointing_y,
                                                   -10),
  });
  REQUIRE(event_queue.get_events() == expected);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
