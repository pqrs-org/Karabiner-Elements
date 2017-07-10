#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                 \
                           TIME_STAMP,                                 \
                           EVENT,                                      \
                           krbn::event_type::EVENT_TYPE,               \
                           EVENT);

#define ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE)           \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                                     \
                           TIME_STAMP,                                                                     \
                           krbn::event_queue::queued_event::event::make_event_from_ignored_device_event(), \
                           krbn::event_type::EVENT_TYPE,                                                   \
                           EVENT);

#define ENQUEUE_LAZY_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                      \
                           TIME_STAMP,                                      \
                           EVENT,                                           \
                           krbn::event_type::EVENT_TYPE,                    \
                           EVENT,                                           \
                           true);

#define ENQUEUE_KEYBOARD_EVENT(EVENTS, KEY_CODE, VALUE, TIME_STAMP)                            \
  {                                                                                            \
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;      \
    keyboard_event.usage_page = *(krbn::types::get_usage_page(krbn::key_code::KEY_CODE));      \
    keyboard_event.usage = *(krbn::types::get_usage(krbn::key_code::KEY_CODE));                \
    keyboard_event.value = VALUE;                                                              \
    EVENTS.push_back(post_event_to_virtual_devices::queue::event(keyboard_event, TIME_STAMP)); \
  }

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
auto event_from_ignored_device_event = krbn::event_queue::queued_event::event::make_event_from_ignored_device_event();

uint64_t modifier_wait = krbn::time_utility::nano_to_absolute(NSEC_PER_MSEC);
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
    uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);

    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, left_shift_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 2, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, spacebar_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, escape_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, button1_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, pointing_x_m10_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, button1_event, key_up);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += interval, pointing_y_10_event, key_down);

    connector.manipulate();

    {
      std::vector<post_event_to_virtual_devices::queue::event> expected;
      time_stamp = 0;
      ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp += interval);
      ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp += interval);

      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += interval));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.buttons[0] = 0x1;
        pointing_input.x = -10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += interval));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += interval));
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input;
        pointing_input.y = 10;
        expected.push_back(post_event_to_virtual_devices::queue::event(pointing_input, time_stamp += interval));
      }

      REQUIRE(manipulator->get_queue().get_events() == expected);
    }

    {
      std::vector<std::pair<krbn::device_id, krbn::key_code>> expected({
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
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, interval * 5);

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

    time_stamp = 0;

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, return_or_enter, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 1, time_stamp += interval);

    REQUIRE(manipulator->get_queue().get_events() == expected);

    ENQUEUE_EVENT(input_event_queue, 1, interval * 6, device_ungrabbed_event, key_down);
    ENQUEUE_EVENT(input_event_queue, 3, interval * 6, device_ungrabbed_event, key_down);

    connector.manipulate();

    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, interval * 6);
    ENQUEUE_KEYBOARD_EVENT(expected, return_or_enter, 0, interval * 6);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp + modifier_wait); // need to wait after 'kHIDUsage_KeyboardA key_down'

    REQUIRE(manipulator->get_queue().get_events() == expected);

    ENQUEUE_EVENT(input_event_queue, 2, interval * 8, device_ungrabbed_event, key_down);

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

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

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

    time_stamp = 0;

    std::vector<post_event_to_virtual_devices::queue::event> expected;
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp + modifier_wait);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp + modifier_wait);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp + modifier_wait);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp += interval);
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

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

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

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp += interval);
    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp += interval);

    time_stamp += interval;
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp += interval);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp += interval);

    REQUIRE(manipulator->get_queue().get_events() == expected);
  }
}

namespace {
class actual_examples_helper final {
public:
  actual_examples_helper(const std::string& file_name) {
    std::ifstream input(std::string("json/") + file_name);
    auto json = nlohmann::json::parse(input);
    for (const auto& j : json) {
      krbn::core_configuration::profile::complex_modifications::parameters parameters;
      if (j.find("parameters") != j.end()) {
        parameters.update(j["parameters"]);
      }
      auto m = krbn::manipulator::manipulator_factory::make_manipulator(j, parameters);
      modifications_manipulator_manager_.push_back_manipulator(m);
    }

    manipulator_ = std::make_shared<post_event_to_virtual_devices>();
    post_event_to_virtual_devices_manipulator_manager_.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator_));

    connector_.emplace_back_connection(modifications_manipulator_manager_,
                                       input_event_queue_,
                                       middle_event_queue_);
    connector_.emplace_back_connection(post_event_to_virtual_devices_manipulator_manager_,
                                       output_event_queue_);
  }

  krbn::event_queue& get_input_event_queue(void) {
    return input_event_queue_;
  }

  void manipulate(void) {
    connector_.manipulate();
  }

  const std::vector<post_event_to_virtual_devices::queue::event> get_events(void) const {
    std::vector<post_event_to_virtual_devices::queue::event> events;
    for (const auto& e : manipulator_->get_queue().get_events()) {
      switch (e.get_type()) {
        case post_event_to_virtual_devices::queue::event::type::keyboard_event:
          events.emplace_back(*(e.get_keyboard_event()), 0);
          break;
        case post_event_to_virtual_devices::queue::event::type::pointing_input:
          events.emplace_back(*(e.get_pointing_input()), 0);
          break;
      }
    }
    return events;
  }

private:
  krbn::manipulator::manipulator_manager modifications_manipulator_manager_;
  krbn::manipulator::manipulator_manager post_event_to_virtual_devices_manipulator_manager_;
  std::shared_ptr<post_event_to_virtual_devices> manipulator_;
  krbn::event_queue input_event_queue_;
  krbn::event_queue middle_event_queue_;
  krbn::event_queue output_event_queue_;
  krbn::manipulator::manipulator_managers_connector connector_;
};
} // namespace

TEST_CASE("actual examples") {
  uint64_t time_stamp = 0;
  uint64_t interval = krbn::time_utility::nano_to_absolute(NSEC_PER_MSEC) * 2;
  std::vector<post_event_to_virtual_devices::queue::event> expected;

  // ----------------------------------------
  // not manipulate target key (tab)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // device_keys_are_released

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp, device_keys_are_released_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp, left_shift_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // "f1 -> f2 (key to key)",

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, f2, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f2, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // f2 -> left_shift

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f2_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f2_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // f3 -> left_command+spacebar (key to modifier+key)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f3_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f3_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f3_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f3_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // f4 -> left_command+left_option (key to modifier+modifier)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f4_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f4_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f4_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f4_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // left_shift+f5 -> spacebar (modifier+key to key)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f5_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f5_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f5_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f5_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // left_command+f6 -> fn (modifier+key to modifier)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f6_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f6_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f6_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f6_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_option+f7 -> right_command+spacebar (modifier+key to modifier+key)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f7_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f7_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f7_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f7_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, spacebar, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_option+f8 -> right_command+right_control (modifier+key to modifier+modifier)

  {
    actual_examples_helper helper("from_key.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f8_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f8_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f8_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f8_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_shift -> f1 (modifier -> key)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_shift_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_shift_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, f1, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f1, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // left_shift -> fn (modifier -> modifier)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // left_control -> fn+f3 (modifier -> modifier+key)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f3, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f3, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f3, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f3, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // left_command -> fn+right_option (modifier -> modifier+modifier)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_command_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_command+right_option -> f5 (modifier+modifier -> key)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f5, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f5, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f5, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f5, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_control+right_option -> left_shift (modifier+modifier -> modifier)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_option_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_control+right_command -> left_shift+f7 (modifier+modifier -> modifier+key)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f7, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f7, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f7, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f7, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_control, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_command+right_control -> left_shift+left_option (modifier+modifier -> modifier+modifier)

  {
    actual_examples_helper helper("from_modifier.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_command_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, right_command, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // right_control -> left_command (escape)

  {
    actual_examples_helper helper("alone.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);
    uint64_t timeout1 = krbn::time_utility::nano_to_absolute(999 * NSEC_PER_MSEC);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += timeout1, right_control_event, key_up);
    uint64_t timeout2 = krbn::time_utility::nano_to_absolute(1001 * NSEC_PER_MSEC);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += timeout2, right_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  {
    actual_examples_helper helper("alone.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 2, time_stamp += interval, spacebar_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 2, time_stamp += interval, pointing_vertical_wheel_100_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 2, time_stamp += interval, spacebar_event, key_up);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, pointing_x_m10_event, key_down);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, pointing_vertical_wheel_0_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  {
    actual_examples_helper helper("alone.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, right_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f2, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, f2, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_command, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, escape, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // fn+up_arrow to fn+page_up (from modifiers == to modifiers)

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, fn_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, fn_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // fn+up_arrow to fn+page_up (from modifiers == to modifiers)
  // (release fn before up_arrow)

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, fn_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, fn_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, up_arrow_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, fn, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, fn, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, page_up, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+p to up_arrow (from modifiers.optional)

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+p to up_arrow (from modifiers.optional)
  // event_from_ignored_device

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT_FROM_IGNORED_DEVICE_EVENT(helper.get_input_event_queue(), 2, time_stamp += interval, spacebar_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+p to up_arrow (from modifiers.optional)
  // control-p, tab

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+p to up_arrow (from modifiers.optional)
  // control-option-p

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_option_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_option_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_option, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+p to up_arrow (from modifiers.optional)
  // control-shift-p

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_shift_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, p_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, up_arrow, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, p, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+u to shift+control+a,delete_or_backspace (multiple to keys)

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, u_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, u_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, u_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, u_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }

  // ----------------------------------------
  // control+f1 to shift+control+a,delete_or_backspace,vk_none (multiple to keys)

  {
    actual_examples_helper helper("complex_modifications.json");

    time_stamp = 0;

    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_down);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, tab_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, left_control_event, key_up);
    ENQUEUE_EVENT(helper.get_input_event_queue(), 1, time_stamp += interval, f1_event, key_up);

    helper.manipulate();

    time_stamp = 0;
    expected.clear();

    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, a, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_shift, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, delete_or_backspace, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 1, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, tab, 0, time_stamp);
    ENQUEUE_KEYBOARD_EVENT(expected, left_control, 0, time_stamp);

    REQUIRE(helper.get_events() == expected);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
