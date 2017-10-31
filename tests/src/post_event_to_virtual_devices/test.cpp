#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE) \
  QUEUE->emplace_back_event(krbn::device_id(DEVICE_ID),                \
                            TIME_STAMP,                                \
                            EVENT,                                     \
                            krbn::event_type::EVENT_TYPE,              \
                            EVENT);

#define ENQUEUE_LAZY_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE) \
  QUEUE->emplace_back_event(krbn::device_id(DEVICE_ID),                     \
                            TIME_STAMP,                                     \
                            EVENT,                                          \
                            krbn::event_type::EVENT_TYPE,                   \
                            EVENT,                                          \
                            true);

#define ENQUEUE_KEYBOARD_EVENT(EVENTS, KEY_CODE, VALUE, TIME_STAMP)                                                                                         \
  {                                                                                                                                                         \
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;                                                                   \
    keyboard_event.usage_page = static_cast<pqrs::karabiner_virtual_hid_device::usage_page>(*(krbn::types::make_hid_usage_page(krbn::key_code::KEY_CODE))); \
    keyboard_event.usage = static_cast<pqrs::karabiner_virtual_hid_device::usage>(*(krbn::types::make_hid_usage(krbn::key_code::KEY_CODE)));                \
    keyboard_event.value = VALUE;                                                                                                                           \
    EVENTS.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, TIME_STAMP));                                                              \
  }

#define ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(EVENTS, TIME_STAMP) \
  EVENTS.push_back(post_event_to_virtual_devices::queue::event::make_clear_keyboard_modifier_flags_event(TIME_STAMP));

using krbn::manipulator::details::post_event_to_virtual_devices;

namespace {
krbn::event_queue::queued_event::event a_event(krbn::key_code::a);
krbn::event_queue::queued_event::event down_arrow_event(krbn::key_code::down_arrow);
krbn::event_queue::queued_event::event escape_event(krbn::key_code::escape);
krbn::event_queue::queued_event::event f10_event(krbn::key_code::f10);
krbn::event_queue::queued_event::event f11_event(krbn::key_code::f11);
krbn::event_queue::queued_event::event f12_event(krbn::key_code::f12);
krbn::event_queue::queued_event::event f1_event(krbn::key_code::f1);
krbn::event_queue::queued_event::event f2_event(krbn::key_code::f2);
krbn::event_queue::queued_event::event f3_event(krbn::key_code::f3);
krbn::event_queue::queued_event::event f4_event(krbn::key_code::f4);
krbn::event_queue::queued_event::event f5_event(krbn::key_code::f5);
krbn::event_queue::queued_event::event f6_event(krbn::key_code::f6);
krbn::event_queue::queued_event::event f7_event(krbn::key_code::f7);
krbn::event_queue::queued_event::event f8_event(krbn::key_code::f8);
krbn::event_queue::queued_event::event f9_event(krbn::key_code::f9);
krbn::event_queue::queued_event::event fn_event(krbn::key_code::fn);
krbn::event_queue::queued_event::event keypad_asterisk_event(krbn::key_code::keypad_asterisk);
krbn::event_queue::queued_event::event keypad_slash_event(krbn::key_code::keypad_slash);
krbn::event_queue::queued_event::event left_command_event(krbn::key_code::left_command);
krbn::event_queue::queued_event::event left_control_event(krbn::key_code::left_control);
krbn::event_queue::queued_event::event left_option_event(krbn::key_code::left_option);
krbn::event_queue::queued_event::event left_shift_event(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event p_event(krbn::key_code::p);
krbn::event_queue::queued_event::event page_up_event(krbn::key_code::page_up);
krbn::event_queue::queued_event::event return_or_enter_event(krbn::key_code::return_or_enter);
krbn::event_queue::queued_event::event right_command_event(krbn::key_code::right_command);
krbn::event_queue::queued_event::event right_control_event(krbn::key_code::right_control);
krbn::event_queue::queued_event::event right_option_event(krbn::key_code::right_option);
krbn::event_queue::queued_event::event right_shift_event(krbn::key_code::right_shift);
krbn::event_queue::queued_event::event spacebar_event(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event tab_event(krbn::key_code::tab);
krbn::event_queue::queued_event::event u_event(krbn::key_code::u);
krbn::event_queue::queued_event::event up_arrow_event(krbn::key_code::up_arrow);

krbn::event_queue::queued_event::event button1_event(krbn::pointing_button::button1);
krbn::event_queue::queued_event::event pointing_x_m10_event(krbn::event_queue::queued_event::event::type::pointing_x, -10);
krbn::event_queue::queued_event::event pointing_y_10_event(krbn::event_queue::queued_event::event::type::pointing_y, 10);
krbn::event_queue::queued_event::event pointing_vertical_wheel_0_event(krbn::event_queue::queued_event::event::type::pointing_vertical_wheel, 0);
krbn::event_queue::queued_event::event pointing_vertical_wheel_100_event(krbn::event_queue::queued_event::event::type::pointing_vertical_wheel, 100);
auto device_ungrabbed_event = krbn::event_queue::queued_event::event::make_device_ungrabbed_event();
auto device_keys_are_released_event = krbn::event_queue::queued_event::event::make_device_keys_are_released_event();

uint64_t modifier_wait = krbn::time_utility::nano_to_absolute(5 * NSEC_PER_MSEC);
} // namespace

TEST_CASE("generic") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, button1_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, pointing_x_m10_event, single);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, button1_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, pointing_y_10_event, single);

    connector.manipulate();

    {
      std::vector<post_event_to_virtual_devices::queue::event> expected;
      time_stamp = 0;

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
      ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);

      time_stamp += interval;
      ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);

      time_stamp += interval;
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp));
      }

      time_stamp += interval;
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        pointing_input.x = -10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp));
      }

      time_stamp += interval;
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp));
      }

      time_stamp += interval;
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.y = 10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp));
      }

      REQUIRE(manipulator->get_queue().get_events() == expected);
    }

    {
      std::vector<std::pair<krbn::device_id, std::pair<krbn::hid_usage_page, krbn::hid_usage>>> expected({
          std::make_pair(krbn::device_id(1), std::make_pair(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(kHIDUsage_KeyboardTab))),
      });
      REQUIRE(manipulator->get_key_event_dispatcher().get_pressed_keys() == expected);
    }
  }
}

TEST_CASE("Same modifier twice from different devices") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_up);

    connector.manipulate();

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, interval);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    time_stamp = interval * 3;

    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, left_shift_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);

    connector.manipulate();

    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, interval * 4);
    ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, interval * 4);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, interval * 5);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

TEST_CASE("device_ungrabbed event") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, escape_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, return_or_enter_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, a_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, a_event, key_down); // `key_code::a` is pressed until device_id(2) is ungrabbed.

    connector.manipulate();

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    time_stamp = 0;

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, return_or_enter, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, a, 1, time_stamp);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    ENQUEUE_EVENT(input_event_queue, 1, interval * 6, device_ungrabbed_event, single);
    ENQUEUE_EVENT(input_event_queue, 3, interval * 6, device_ungrabbed_event, single);

    connector.manipulate();

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, interval * 6);
    ENQUEUE_KEYBOARD_EVENT(expected, return_or_enter, 0, interval * 6);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp - interval + modifier_wait); // need to wait after 'kHIDUsage_KeyboardA key_down'
    ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, interval * 6);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    ENQUEUE_EVENT(input_event_queue, 2, interval * 8, device_ungrabbed_event, single);

    connector.manipulate();

    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, interval * 8);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 0, interval * 8);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

TEST_CASE("wait around modifier") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, tab_event, key_down);
    // left_shift_event, escape_event (key_down)
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp + 1, escape_event, key_down);
    // left_shift_event, escape_event (key_up)
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp + 1, left_shift_event, key_up);
    // escape_event, left_shift_event (key_down)
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp + 1, left_shift_event, key_down);
    // escape_event, left_shift_event (key_up)
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp + 1, escape_event, key_up);

    connector.manipulate();

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    time_stamp = 0;

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp + modifier_wait);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp + modifier_wait);
    ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, time_stamp + 1);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp + modifier_wait);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp + modifier_wait);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

TEST_CASE("lazy events") {
  {
    // ----------------------------------------
    // Collapse lazy events
    //   * left_shift:   key_down,key_up
    //   * left_control: key_up,key_down

    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_LAZY_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_control_event, key_up);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, fn_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, time_stamp += interval, fn_event, key_up);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_up);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, fn_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, fn_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_up);

    connector.manipulate();

    // ----------------------------------------

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    time_stamp = 0;

    time_stamp += interval;
    time_stamp += interval;
    time_stamp += interval;
    time_stamp += interval;

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);

    time_stamp += interval;

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);

    time_stamp += interval;

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_CLEAR_KEYBOARD_MODIFIER_FLAGS_EVENT(expected, time_stamp);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

TEST_CASE("actual examples") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/tests.json")));
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
