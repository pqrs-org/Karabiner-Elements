#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/details/add_delay_after_modifier_key_down.hpp"
#include "manipulator/details/collapse_lazy_events.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "thread_utility.hpp"

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                 \
                           TIME_STAMP,                                                 \
                           EVENT,                                                      \
                           krbn::event_type::EVENT_TYPE,                               \
                           ORIGINAL_EVENT);

#define ENQUEUE_KEYBOARD_EVENT(EVENTS, USAGE, VALUE, TIME_STAMP)                               \
  {                                                                                            \
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;      \
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(USAGE);                   \
    keyboard_event.value = VALUE;                                                              \
    EVENTS.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, TIME_STAMP)); \
  }

using krbn::manipulator::details::post_event_to_virtual_devices;

namespace {
krbn::event_queue::queued_event::event left_shift_event(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event spacebar_event(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event escape_event(krbn::key_code::escape);
krbn::event_queue::queued_event::event tab_event(krbn::key_code::tab);
krbn::event_queue::queued_event::event button1_event(krbn::pointing_button::button1);
krbn::event_queue::queued_event::event pointing_x_m10_event(krbn::event_queue::queued_event::event::type::pointing_x, -10);
krbn::event_queue::queued_event::event pointing_y_10_event(krbn::event_queue::queued_event::event::type::pointing_y, 10);
krbn::event_queue::queued_event::event device_ungrabbed_event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1);
} // namespace

TEST_CASE("generic") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, escape_event, key_down, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, escape_event, key_up, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, tab_event, key_down, tab_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, button1_event, key_down, button1_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, pointing_x_m10_event, key_down, pointing_x_m10_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, button1_event, key_up, button1_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, pointing_y_10_event, key_down, pointing_y_10_event);

    connector.manipulate(time_stamp += 100);

    {
      std::vector<post_event_to_virtual_devices::queue::event> expected;
      time_stamp = 0;
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardLeftShift, 1, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardLeftShift, 0, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardSpacebar, 1, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardSpacebar, 0, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardEscape, 1, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardEscape, 0, time_stamp += 100);
      ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardTab, 1, time_stamp += 100);

      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        pointing_input.x = -10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.y = 10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += 100));
      }

      REQUIRE(manipulator->get_queue().get_events() == expected);
    }

    {
      std::unordered_set<post_event_to_virtual_devices::key_event_dispatcher::pressed_key,
                         post_event_to_virtual_devices::key_event_dispatcher::pressed_key_hash>
          expected({
              std::make_pair(krbn::device_id(1), krbn::key_code::tab),
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

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_up, left_shift_event);

    connector.manipulate(time_stamp += 100);

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardLeftShift, 1, 100);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    time_stamp = 300;

    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += 100, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_down, left_shift_event);

    connector.manipulate(time_stamp += 100);

    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardLeftShift, 0, 400);
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardLeftShift, 1, 500);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

TEST_CASE("device_ungrabbed event") {
  {
    krbn::manipulator::manipulator_manager manipulator_manager;

    auto manipulator = std::make_shared<post_event_to_virtual_devices>();
    manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    uint64_t time_stamp = 0;

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, tab_event, key_down, tab_event);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += 100, escape_event, key_down, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, tab_event, key_up, tab_event);

    connector.manipulate(500);

    time_stamp = 0;

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardTab, 1, time_stamp += 100);
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardEscape, 1, time_stamp += 100);
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardSpacebar, 1, time_stamp += 100);
    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardTab, 0, time_stamp += 100);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    ENQUEUE_EVENT(input_event_queue, 1, 600, device_ungrabbed_event, key_down, device_ungrabbed_event);
    ENQUEUE_EVENT(input_event_queue, 3, 600, device_ungrabbed_event, key_down, device_ungrabbed_event);

    connector.manipulate(700);

    ENQUEUE_KEYBOARD_EVENT(expected, kHIDUsage_KeyboardSpacebar, 0, 600);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
