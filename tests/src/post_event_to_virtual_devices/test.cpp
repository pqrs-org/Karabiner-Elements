#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/details/add_delay_after_modifier_key_down.hpp"
#include "manipulator/details/collapse_lazy_events.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulator.details.post_event_to_virtual_devices") {
  using krbn::manipulator::details::post_event_to_virtual_devices;

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
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(2),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::pointing_button::button1));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::pointing_x, -10),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::pointing_x, -10));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::pointing_button::button1),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::pointing_button::button1));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         time_stamp += 100,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::pointing_y, 10),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::pointing_y, 10));
    connector.manipulate(time_stamp += 100);

    {
      std::vector<post_event_to_virtual_devices::queue::event> expected;
      time_stamp = 0;
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardLeftShift);
        keyboard_event.value = 1;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardLeftShift);
        keyboard_event.value = 0;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
        keyboard_event.value = 1;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
        keyboard_event.value = 0;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardEscape);
        keyboard_event.value = 1;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardEscape);
        keyboard_event.value = 0;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardTab);
        keyboard_event.value = 1;
        expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, time_stamp += 100));
      }
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

  // Same modifier twice from different devices

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

    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(2),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));

    connector.manipulate(400);

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardLeftShift);
      keyboard_event.value = 1;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 100));
    }

    REQUIRE(manipulator->get_queue().get_events() == expected);

    input_event_queue.emplace_back_event(krbn::device_id(2),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         500,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));

    connector.manipulate(600);

    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardLeftShift);
      keyboard_event.value = 0;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 400));
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardLeftShift);
      keyboard_event.value = 1;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 500));
    }

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }

  // device_ungrabbed event

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

    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab));
    input_event_queue.emplace_back_event(krbn::device_id(2),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab));

    connector.manipulate(500);

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardTab);
      keyboard_event.value = 1;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 100));
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardEscape);
      keyboard_event.value = 1;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 200));
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
      keyboard_event.value = 1;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 300));
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardTab);
      keyboard_event.value = 0;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 400));
    }

    REQUIRE(manipulator->get_queue().get_events() == expected);

    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         600,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1));
    input_event_queue.emplace_back_event(krbn::device_id(3),
                                         600,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1));

    connector.manipulate(700);

    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
      keyboard_event.value = 0;
      expected.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, 600));
    }

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
