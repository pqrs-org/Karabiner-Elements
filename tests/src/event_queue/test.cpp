#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "event_queue.hpp"
#include "thread_utility.hpp"

namespace {
krbn::event_queue::queued_event::event event_a(*(krbn::types::get_key_code("a")));
krbn::event_queue::queued_event::event event_escape(*(krbn::types::get_key_code("escape")));
krbn::event_queue::queued_event::event event_left_shift(*(krbn::types::get_key_code("left_shift")));
krbn::event_queue::queued_event::event event_right_shift(*(krbn::types::get_key_code("right_shift")));
krbn::event_queue::queued_event::event event_spacebar(*(krbn::types::get_key_code("spacebar")));
krbn::event_queue::queued_event::event event_tab(*(krbn::types::get_key_code("tab")));

krbn::event_queue::queued_event::event event_button2(krbn::pointing_button::button2);

krbn::event_queue::queued_event::event event_x_p10(krbn::event_queue::queued_event::event::type::pointing_x, 10);
krbn::event_queue::queued_event::event event_y_m10(krbn::event_queue::queued_event::event::type::pointing_y, -10);
} // namespace

TEST_CASE("emplace_back_event") {
  // normal order
  {
    krbn::event_queue event_queue;

    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_a,
                                   krbn::event_type::key_down,
                                   event_a);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   200,
                                   event_left_shift,
                                   krbn::event_type::key_down,
                                   event_left_shift);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == true);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_button2,
                                   krbn::event_type::key_down,
                                   event_button2);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(krbn::pointing_button::button2) == true);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   400,
                                   event_left_shift,
                                   krbn::event_type::key_up,
                                   event_left_shift);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   500,
                                   event_a,
                                   krbn::event_type::key_up,
                                   event_a);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   600,
                                   event_button2,
                                   krbn::event_type::key_up,
                                   event_button2);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(krbn::pointing_button::button2) == false);

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_a,
                                        krbn::event_type::key_down,
                                        event_a),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        200,
                                        event_left_shift,
                                        krbn::event_type::key_down,
                                        event_left_shift),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_button2,
                                        krbn::event_type::key_down,
                                        event_button2),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        400,
                                        event_left_shift,
                                        krbn::event_type::key_up,
                                        event_left_shift),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        500,
                                        event_a,
                                        krbn::event_type::key_up,
                                        event_a),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        600,
                                        event_button2,
                                        krbn::event_type::key_up,
                                        event_button2),
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
                                   event_a,
                                   krbn::event_type::key_down,
                                   event_a);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_left_shift,
                                   krbn::event_type::key_down,
                                   event_left_shift);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   200,
                                   event_spacebar,
                                   krbn::event_type::key_down,
                                   event_spacebar);
    // push "a" and "left_shift" (key_up) at the same time.
    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_left_shift,
                                   krbn::event_type::key_up,
                                   event_left_shift);
    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   event_a,
                                   krbn::event_type::key_up,
                                   event_a);

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_left_shift,
                                        krbn::event_type::key_down,
                                        event_left_shift),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_a,
                                        krbn::event_type::key_down,
                                        event_a),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        200,
                                        event_spacebar,
                                        krbn::event_type::key_down,
                                        event_spacebar),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_a,
                                        krbn::event_type::key_up,
                                        event_a),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        event_left_shift,
                                        krbn::event_type::key_up,
                                        event_left_shift),
    });
    REQUIRE(event_queue.get_events() == expected);
  }
}

TEST_CASE("compare") {
  krbn::event_queue::queued_event spacebar_down(krbn::device_id(1),
                                                100,
                                                event_spacebar,
                                                krbn::event_type::key_down,
                                                event_spacebar);
  krbn::event_queue::queued_event right_shift_down(krbn::device_id(1),
                                                   100,
                                                   event_right_shift,
                                                   krbn::event_type::key_down,
                                                   event_right_shift);
  krbn::event_queue::queued_event escape_down(krbn::device_id(1),
                                              200,
                                              event_escape,
                                              krbn::event_type::key_down,
                                              event_escape);
  krbn::event_queue::queued_event spacebar_up(krbn::device_id(1),
                                              300,
                                              event_spacebar,
                                              krbn::event_type::key_up,
                                              event_spacebar);
  krbn::event_queue::queued_event right_shift_up(krbn::device_id(1),
                                                 300,
                                                 event_right_shift,
                                                 krbn::event_type::key_up,
                                                 event_right_shift);

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
                                      event_tab,
                                      krbn::event_type::key_down,
                                      event_tab),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      200,
                                      event_tab,
                                      krbn::event_type::key_up,
                                      event_tab),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      300,
                                      event_button2,
                                      krbn::event_type::key_down,
                                      event_button2),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      400,
                                      event_button2,
                                      krbn::event_type::key_up,
                                      event_button2),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      500,
                                      event_x_p10,
                                      krbn::event_type::key_down,
                                      event_x_p10),
      krbn::event_queue::queued_event(krbn::device_id(1),
                                      600,
                                      event_y_m10,
                                      krbn::event_type::key_down,
                                      event_y_m10),
  });
  REQUIRE(event_queue.get_events() == expected);
}

TEST_CASE("increase_time_stamp_delay") {
  {
    krbn::event_queue event_queue;

    event_queue.emplace_back_event(krbn::device_id(1),
                                   100,
                                   event_tab,
                                   krbn::event_type::key_down,
                                   event_tab);

    event_queue.increase_time_stamp_delay(10);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   200,
                                   event_tab,
                                   krbn::event_type::key_up,
                                   event_tab);

    event_queue.emplace_back_event(krbn::device_id(1),
                                   300,
                                   krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                   krbn::hid_usage(kHIDUsage_KeyboardTab),
                                   1);

    event_queue.push_back_event(krbn::event_queue::queued_event(krbn::device_id(1),
                                                                400,
                                                                event_tab,
                                                                krbn::event_type::key_up,
                                                                event_tab));

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        event_tab,
                                        krbn::event_type::key_down,
                                        event_tab),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        210,
                                        event_tab,
                                        krbn::event_type::key_up,
                                        event_tab),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        310,
                                        event_tab,
                                        krbn::event_type::key_down,
                                        event_tab),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        410,
                                        event_tab,
                                        krbn::event_type::key_up,
                                        event_tab),
    });

    REQUIRE(event_queue.get_events() == expected);

    while (!event_queue.empty()) {
      event_queue.erase_front_event();
    }
    REQUIRE(event_queue.get_time_stamp_delay() == 0);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
