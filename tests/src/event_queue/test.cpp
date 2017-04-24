#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "event_queue.hpp"
#include "thread_utility.hpp"

namespace {
krbn::event_queue::queued_event::event event_a_down(*(krbn::types::get_key_code("a")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_a_up(*(krbn::types::get_key_code("a")), krbn::event_type::key_up);
krbn::event_queue::queued_event::event event_escape_down(*(krbn::types::get_key_code("escape")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_escape_up(*(krbn::types::get_key_code("escape")), krbn::event_type::key_up);
krbn::event_queue::queued_event::event event_left_shift_down(*(krbn::types::get_key_code("left_shift")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_left_shift_up(*(krbn::types::get_key_code("left_shift")), krbn::event_type::key_up);
krbn::event_queue::queued_event::event event_right_shift_down(*(krbn::types::get_key_code("right_shift")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_right_shift_up(*(krbn::types::get_key_code("right_shift")), krbn::event_type::key_up);
krbn::event_queue::queued_event::event event_spacebar_down(*(krbn::types::get_key_code("spacebar")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_spacebar_up(*(krbn::types::get_key_code("spacebar")), krbn::event_type::key_up);
krbn::event_queue::queued_event::event event_tab_down(*(krbn::types::get_key_code("tab")), krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_tab_up(*(krbn::types::get_key_code("tab")), krbn::event_type::key_up);

krbn::event_queue::queued_event::event event_button2_down(krbn::pointing_button::button2, krbn::event_type::key_down);
krbn::event_queue::queued_event::event event_button2_up(krbn::pointing_button::button2, krbn::event_type::key_up);

krbn::event_queue::queued_event::event event_x_p10(krbn::event_queue::queued_event::event::type::pointing_x, 10);
krbn::event_queue::queued_event::event event_y_m10(krbn::event_queue::queued_event::event::type::pointing_y, -10);
} // namespace

TEST_CASE("event") {
  {
    krbn::event_queue::queued_event::event event = event_a_down;
    event.invert_event_type();
    REQUIRE(event == event_a_up);
  }
  {
    krbn::event_queue::queued_event::event event = event_button2_up;
    event.invert_event_type();
    REQUIRE(event == event_button2_down);
  }
  {
    krbn::event_queue::queued_event::event event = event_x_p10;
    event.invert_event_type();
    REQUIRE(event == event_x_p10);
  }
}

TEST_CASE("emplace_back_event") {
  // normal order
  {
    krbn::event_queue event_queue;

    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_a_down,
                                   event_a_down);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   200,
                                   event_left_shift_down,
                                   event_left_shift_down);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_left_shift_up,
                                   event_left_shift_up);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   400,
                                   event_a_up,
                                   event_a_up);

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_a_down,
                                        event_a_down),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        200,
                                        event_left_shift_down,
                                        event_left_shift_down),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_left_shift_up,
                                        event_left_shift_up),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        400,
                                        event_a_up,
                                        event_a_up),
    });
    REQUIRE(event_queue.get_events() == expected);

    REQUIRE(event_queue.get_events()[0].get_valid() == true);
    REQUIRE(event_queue.get_events()[0].get_lazy() == false);
  }

  // reorder events
  {
    krbn::event_queue event_queue;

    // push "a" and "left_shift" (key_down) at the same time.
    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_a_down,
                                   event_a_down);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_left_shift_down,
                                   event_left_shift_down);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   200,
                                   event_spacebar_down,
                                   event_spacebar_down);
    // push "a" and "left_shift" (key_up) at the same time.
    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_left_shift_up,
                                   event_left_shift_up);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_a_up,
                                   event_a_up);

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_left_shift_down,
                                        event_left_shift_down),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_a_down,
                                        event_a_down),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        200,
                                        event_spacebar_down,
                                        event_spacebar_down),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_a_up,
                                        event_a_up),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_left_shift_up,
                                        event_left_shift_up),
    });
    REQUIRE(event_queue.get_events() == expected);
  }
}

TEST_CASE("compare") {
  krbn::event_queue::queued_event spacebar_down(krbn::device_id(1),
                                                100,
                                                event_spacebar_down,
                                                event_spacebar_down);
  krbn::event_queue::queued_event right_shift_down(krbn::device_id(1),
                                                   100,
                                                   event_right_shift_down,
                                                   event_right_shift_down);
  krbn::event_queue::queued_event escape_down(krbn::device_id(1),
                                              200,
                                              event_escape_down,
                                              event_escape_down);
  krbn::event_queue::queued_event spacebar_up(krbn::device_id(1),
                                              300,
                                              event_spacebar_up,
                                              event_spacebar_up);
  krbn::event_queue::queued_event right_shift_up(krbn::device_id(1),
                                                 300,
                                                 event_right_shift_up,
                                                 event_right_shift_up);

  REQUIRE(krbn::event_queue::compare(spacebar_down, spacebar_down) == false);

  REQUIRE(krbn::event_queue::compare(spacebar_down, escape_down) == true);
  REQUIRE(krbn::event_queue::compare(escape_down, spacebar_down) == false);

  REQUIRE(krbn::event_queue::compare(spacebar_down, right_shift_down) == false);
  REQUIRE(krbn::event_queue::compare(right_shift_down, spacebar_down) == true);

  REQUIRE(krbn::event_queue::compare(spacebar_down, right_shift_up) == true);
  REQUIRE(krbn::event_queue::compare(right_shift_up, spacebar_down) == false);

  REQUIRE(krbn::event_queue::compare(spacebar_up, right_shift_up) == true);
  REQUIRE(krbn::event_queue::compare(right_shift_up, spacebar_up) == false);

  REQUIRE(krbn::event_queue::compare(spacebar_up, right_shift_down) == false);
  REQUIRE(krbn::event_queue::compare(right_shift_down, spacebar_up) == true);
}

TEST_CASE("emplace_back_event.usage_page") {
  krbn::event_queue event_queue;

  event_queue.emplace_back_event(krbn::device_id(1),
                                 0,
                                 krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                 krbn::hid_usage(kHIDUsage_KeyboardErrorRollOver),
                                 1);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 100,
                                 krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                 krbn::hid_usage(kHIDUsage_KeyboardTab),
                                 1);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 200,
                                 krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                 krbn::hid_usage(kHIDUsage_KeyboardTab),
                                 0);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 300,
                                 krbn::hid_usage_page(kHIDPage_Button),
                                 krbn::hid_usage(2),
                                 1);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 400,
                                 krbn::hid_usage_page(kHIDPage_Button),
                                 krbn::hid_usage(2),
                                 0);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 500,
                                 krbn::hid_usage_page(kHIDPage_GenericDesktop),
                                 krbn::hid_usage(kHIDUsage_GD_X),
                                 10);
  event_queue.emplace_back_event(krbn::device_id(1),
                                 600,
                                 krbn::hid_usage_page(kHIDPage_GenericDesktop),
                                 krbn::hid_usage(kHIDUsage_GD_Y),
                                 -10);

  std::vector<krbn::event_queue::queued_event> expected({
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      100,
                                      event_tab_down,
                                      event_tab_down),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      200,
                                      event_tab_up,
                                      event_tab_up),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      300,
                                      event_button2_down,
                                      event_button2_down),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      400,
                                      event_button2_up,
                                      event_button2_up),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      500,
                                      event_x_p10,
                                      event_x_p10),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      600,
                                      event_y_m10,
                                      event_y_m10),
  });
  REQUIRE(event_queue.get_events() == expected);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
