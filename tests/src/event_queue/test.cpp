#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "event_queue.hpp"
#include "thread_utility.hpp"

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                 \
                           TIME_STAMP,                                                 \
                           EVENT,                                                      \
                           krbn::event_type::EVENT_TYPE,                               \
                           ORIGINAL_EVENT)

#define ENQUEUE_USAGE(QUEUE, DEVICE_ID, TIME_STAMP, USAGE_PAGE, USAGE, VALUE) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                        \
                           TIME_STAMP,                                        \
                           krbn::hid_usage_page(USAGE_PAGE),                  \
                           krbn::hid_usage(USAGE),                            \
                           VALUE)

#define PUSH_BACK_QUEUED_EVENT(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  VECTOR.push_back(krbn::event_queue::queued_event(krbn::device_id(DEVICE_ID),                   \
                                                   TIME_STAMP,                                   \
                                                   EVENT,                                        \
                                                   krbn::event_type::EVENT_TYPE,                 \
                                                   ORIGINAL_EVENT))

namespace {
krbn::event_queue::queued_event::event event_a(krbn::key_code::a);
krbn::event_queue::queued_event::event event_caps_lock(krbn::key_code::caps_lock);
krbn::event_queue::queued_event::event event_escape(krbn::key_code::escape);
krbn::event_queue::queued_event::event event_left_control(krbn::key_code::left_control);
krbn::event_queue::queued_event::event event_left_shift(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event event_right_control(krbn::key_code::right_control);
krbn::event_queue::queued_event::event event_right_shift(krbn::key_code::right_shift);
krbn::event_queue::queued_event::event event_spacebar(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event event_tab(krbn::key_code::tab);

krbn::event_queue::queued_event::event event_button2(krbn::pointing_button::button2);

krbn::event_queue::queued_event::event event_x_p10(krbn::event_queue::queued_event::event::type::pointing_x, 10);
krbn::event_queue::queued_event::event event_y_m10(krbn::event_queue::queued_event::event::type::pointing_y, -10);

krbn::event_queue::queued_event::event event_caps_lock_state_changed_1(krbn::event_queue::queued_event::event::type::caps_lock_state_changed, 1);
krbn::event_queue::queued_event::event event_caps_lock_state_changed_0(krbn::event_queue::queued_event::event::type::caps_lock_state_changed, 0);
} // namespace

TEST_CASE("emplace_back_event") {
  // Normal order
  {
    krbn::event_queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, event_a, key_down, event_a);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 200, event_left_shift, key_down, event_left_shift);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == true);

    ENQUEUE_EVENT(event_queue, 1, 300, event_button2, key_down, event_button2);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(krbn::pointing_button::button2) == true);

    ENQUEUE_EVENT(event_queue, 1, 400, event_left_shift, key_up, event_left_shift);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 500, event_a, key_up, event_a);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::left_shift) == false);

    ENQUEUE_EVENT(event_queue, 1, 600, event_button2, key_up, event_button2);

    REQUIRE(event_queue.get_pointing_button_manager().is_pressed(krbn::pointing_button::button2) == false);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_a, key_down, event_a);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 200, event_left_shift, key_down, event_left_shift);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, event_button2, key_down, event_button2);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 400, event_left_shift, key_up, event_left_shift);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 500, event_a, key_up, event_a);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 600, event_button2, key_up, event_button2);
    REQUIRE(event_queue.get_events() == expected);

    REQUIRE(event_queue.get_events()[0].get_valid() == true);
    REQUIRE(event_queue.get_events()[0].get_lazy() == false);
  }

  // Reorder events
  {
    krbn::event_queue event_queue;

    // Push `a, left_control, left_shift (key_down)` at the same time.

    ENQUEUE_EVENT(event_queue, 1, 100, event_left_control, key_down, event_left_control);
    ENQUEUE_EVENT(event_queue, 1, 100, event_a, key_down, event_a);
    ENQUEUE_EVENT(event_queue, 1, 100, event_left_shift, key_down, event_left_shift);
    ENQUEUE_EVENT(event_queue, 1, 200, event_spacebar, key_down, event_spacebar);

    // Push `a, left_control, left_shift (key_up)` at the same time.

    ENQUEUE_EVENT(event_queue, 1, 300, event_left_shift, key_up, event_left_shift);
    ENQUEUE_EVENT(event_queue, 1, 300, event_a, key_up, event_a);
    ENQUEUE_EVENT(event_queue, 1, 300, event_left_control, key_up, event_left_control);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_left_control, key_down, event_left_control);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_left_shift, key_down, event_left_shift);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_a, key_down, event_a);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 200, event_spacebar, key_down, event_spacebar);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, event_a, key_up, event_a);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, event_left_shift, key_up, event_left_shift);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, event_left_control, key_up, event_left_control);
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

  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 0, kHIDPage_KeyboardOrKeypad, kHIDUsage_KeyboardErrorRollOver, 1) == false);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 100, kHIDPage_KeyboardOrKeypad, kHIDUsage_KeyboardTab, 1) == true);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 200, kHIDPage_KeyboardOrKeypad, kHIDUsage_KeyboardTab, 0) == true);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 300, kHIDPage_Button, 2, 1) == true);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 400, kHIDPage_Button, 2, 0) == true);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 500, kHIDPage_GenericDesktop, kHIDUsage_GD_X, 10) == true);
  REQUIRE(ENQUEUE_USAGE(event_queue, 1, 600, kHIDPage_GenericDesktop, kHIDUsage_GD_Y, -10) == true);

  std::vector<krbn::event_queue::queued_event> expected;
  PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_tab, key_down, event_tab);
  PUSH_BACK_QUEUED_EVENT(expected, 1, 200, event_tab, key_up, event_tab);
  PUSH_BACK_QUEUED_EVENT(expected, 1, 300, event_button2, key_down, event_button2);
  PUSH_BACK_QUEUED_EVENT(expected, 1, 400, event_button2, key_up, event_button2);
  PUSH_BACK_QUEUED_EVENT(expected, 1, 500, event_x_p10, key_down, event_x_p10);
  PUSH_BACK_QUEUED_EVENT(expected, 1, 600, event_y_m10, key_down, event_y_m10);
  REQUIRE(event_queue.get_events() == expected);
}

TEST_CASE("increase_time_stamp_delay") {
  {
    krbn::event_queue event_queue;

    ENQUEUE_EVENT(event_queue, 1, 100, event_tab, key_down, event_tab);

    event_queue.increase_time_stamp_delay(10);

    ENQUEUE_EVENT(event_queue, 1, 200, event_tab, key_up, event_tab);

    ENQUEUE_USAGE(event_queue, 1, 300, kHIDPage_KeyboardOrKeypad, kHIDUsage_KeyboardTab, 1);

    ENQUEUE_EVENT(event_queue, 1, 400, event_tab, key_up, event_tab);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, event_tab, key_down, event_tab);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 210, event_tab, key_up, event_tab);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 310, event_tab, key_down, event_tab);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 410, event_tab, key_up, event_tab);
    REQUIRE(event_queue.get_events() == expected);

    while (!event_queue.empty()) {
      event_queue.erase_front_event();
    }
    REQUIRE(event_queue.get_time_stamp_delay() == 0);
  }
}

TEST_CASE("caps_lock_state_changed") {
  {
    krbn::event_queue event_queue;

    // modifier_flag_manager's caps lock state will not be changed by key_down event.
    ENQUEUE_EVENT(event_queue, 1, 100, event_caps_lock, key_down, event_caps_lock);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == false);

    ENQUEUE_EVENT(event_queue, 1, 100, event_caps_lock_state_changed_1, key_down, event_caps_lock_state_changed_1);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);

    // Send twice

    ENQUEUE_EVENT(event_queue, 1, 100, event_caps_lock_state_changed_1, key_down, event_caps_lock_state_changed_1);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);

    ENQUEUE_EVENT(event_queue, 1, 100, event_caps_lock_state_changed_0, key_down, event_caps_lock_state_changed_0);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == false);

    ENQUEUE_EVENT(event_queue, 1, 100, event_caps_lock_state_changed_1, key_down, event_caps_lock_state_changed_1);

    REQUIRE(event_queue.get_modifier_flag_manager().is_pressed(krbn::modifier_flag::caps_lock) == true);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
