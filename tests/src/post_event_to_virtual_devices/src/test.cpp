#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "caps_lock_led_override_manager.hpp"
#include "dispatcher_utility.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  "actual examples"_test = [] {
    auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

    helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/tests.json"));

    helper = nullptr;
  };

  "set_caps_lock_led"_test = [] {
    auto caps_lock_led_override_manager = std::make_shared<krbn::caps_lock_led_override_manager>();
    auto manipulator = std::make_shared<krbn::manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices>(
        std::weak_ptr<krbn::console_user_server_peer>(),
        std::weak_ptr<krbn::notification_message_manager>(),
        caps_lock_led_override_manager);

    auto input_event_queue = std::make_shared<krbn::event_queue::queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue::queue>();

    auto manipulate = [&](krbn::caps_lock_led_value value, krbn::event_type event_type) {
      auto e = krbn::event_queue::event::make_set_caps_lock_led_event(value);
      krbn::event_queue::entry entry(krbn::device_id(1),
                                     krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                     e,
                                     event_type,
                                     std::nullopt,
                                     e,
                                     krbn::event_queue::state::original);

      manipulator->manipulate(entry,
                              *input_event_queue,
                              output_event_queue,
                              krbn::absolute_time_point(0));

      // `async_set_override` runs on the dispatcher.
      auto wait = pqrs::make_thread_wait();
      caps_lock_led_override_manager->enqueue_to_dispatcher([wait] {
        wait->notify();
      });
      wait->wait_notice();
    };

    expect(caps_lock_led_override_manager->get_override() == std::nullopt);

    manipulate(krbn::caps_lock_led_value::on, krbn::event_type::key_down);
    expect(caps_lock_led_override_manager->get_override() == krbn::led_state::on);

    // key_up must not change the override, otherwise every `set_caps_lock_led` would
    // be undone as soon as the key is released.
    manipulate(krbn::caps_lock_led_value::off, krbn::event_type::key_up);
    expect(caps_lock_led_override_manager->get_override() == krbn::led_state::on);

    manipulate(krbn::caps_lock_led_value::off, krbn::event_type::key_down);
    expect(caps_lock_led_override_manager->get_override() == krbn::led_state::off);

    manipulate(krbn::caps_lock_led_value::automatic, krbn::event_type::key_down);
    expect(caps_lock_led_override_manager->get_override() == std::nullopt);

    // The events are forwarded to the output queue like the other virtual events.
    expect(output_event_queue->get_entries().size() == 4_ul);
    for (const auto& e : output_event_queue->get_entries()) {
      expect(e.get_event().get_type() == krbn::event_queue::event::type::set_caps_lock_led);
    }

    manipulator = nullptr;
  };

  "mouse_key_handler.count_converter"_test = [] {
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));

      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(1));

      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(-1));
    }
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
    }
  };

  return 0;
}
