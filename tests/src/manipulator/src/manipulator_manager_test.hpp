#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include <boost/ut.hpp>
#include <pqrs/gsl.hpp>

namespace {
class deferred_modifier_key_up_events_test_manipulator final : public krbn::manipulator::manipulators::base {
public:
  explicit deferred_modifier_key_up_events_test_manipulator(std::shared_ptr<std::size_t> count) : count_(count) {
  }

  bool already_manipulated(const krbn::event_queue::entry&) override {
    return false;
  }

  void post_deferred_modifier_key_up_events(const krbn::event_queue::entry&,
                                            krbn::event_queue::queue&,
                                            krbn::manipulator::manipulators::deferred_modifier_key_up_event_scope) override {
    ++*count_;
  }

  krbn::manipulator::manipulate_result manipulate(krbn::event_queue::entry&,
                                                  const krbn::event_queue::queue&,
                                                  std::shared_ptr<krbn::event_queue::queue>,
                                                  krbn::absolute_time_point,
                                                  std::shared_ptr<const krbn::manipulator::manipulators::deferred_output_session>) override {
    return krbn::manipulator::manipulate_result::passed;
  }

  bool active() const override {
    return false;
  }

  bool needs_virtual_hid_pointing() const override {
    return false;
  }

  bool uses_caps_lock_led_override() const override {
    return false;
  }

  void handle_device_keys_and_pointing_buttons_are_released_event(const krbn::event_queue::entry&,
                                                                  krbn::event_queue::queue&) override {
  }

  void handle_device_ungrabbed_event(krbn::device_id,
                                     const krbn::event_queue::queue&,
                                     krbn::absolute_time_point) override {
  }

  void handle_pointing_device_event_from_event_tap(const krbn::event_queue::entry&,
                                                   krbn::event_queue::queue&) override {
  }

private:
  std::shared_ptr<std::size_t> count_;
};
} // namespace

void run_manipulator_manager_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "manipulator.manipulator_manager"_test = [] {
    auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();
    helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/manipulator_manager/tests.json"));

    helper = nullptr;
  };

  "min_input_event_time_stamp"_test = [] {
    std::vector<pqrs::not_null_shared_ptr_t<krbn::event_queue::queue>> event_queues;
    event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
    event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
    event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
    event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
    event_queues.push_back(std::make_shared<krbn::event_queue::queue>());

    krbn::manipulator::manipulator_managers_connector connector;

    std::vector<pqrs::not_null_shared_ptr_t<krbn::manipulator::manipulator_manager>> manipulator_managers;
    for (size_t i = 0; i < event_queues.size() - 1; ++i) {
      manipulator_managers.push_back(std::make_shared<krbn::manipulator::manipulator_manager>());
    }

    for (size_t i = 0; i < manipulator_managers.size(); ++i) {
      connector.emplace_back_connection(pqrs::make_weak(manipulator_managers[i]),
                                        pqrs::make_weak(event_queues[i]),
                                        pqrs::make_weak(event_queues[i + 1]));
    }

    expect(!connector.min_input_event_time_stamp());

    // ----------------------------------------

    auto event = krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                       pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

    event_queues[2]->emplace_back_entry({
        .device_id = krbn::device_id(1),
        .event_time_stamp = krbn::event_queue::event_time_stamp(krbn::absolute_time_point(5000)),
        .event = event,
        .event_type = krbn::event_type::key_down,
        .event_integer_value = krbn::event_integer_value::value_t(1),
        .original_event = event,
        .state = krbn::event_queue::state::original,
    });

    expect(connector.min_input_event_time_stamp() == krbn::absolute_time_point(5000));

    // ----------------------------------------

    event_queues[0]->emplace_back_entry({
        .device_id = krbn::device_id(1),
        .event_time_stamp = krbn::event_queue::event_time_stamp(krbn::absolute_time_point(4000)),
        .event = event,
        .event_type = krbn::event_type::key_down,
        .event_integer_value = krbn::event_integer_value::value_t(1),
        .original_event = event,
        .state = krbn::event_queue::state::original,
    });

    expect(connector.min_input_event_time_stamp() == krbn::absolute_time_point(4000));

    // ----------------------------------------
    // min_input_event_time_stamp uses only the front entry.

    event_queues[3]->emplace_back_entry({
        .device_id = krbn::device_id(1),
        .event_time_stamp = krbn::event_queue::event_time_stamp(krbn::absolute_time_point(3000)),
        .event = event,
        .event_type = krbn::event_type::key_down,
        .event_integer_value = krbn::event_integer_value::value_t(1),
        .original_event = event,
        .state = krbn::event_queue::state::original,
    });
    event_queues[0]->emplace_back_entry({
        .device_id = krbn::device_id(1),
        .event_time_stamp = krbn::event_queue::event_time_stamp(krbn::absolute_time_point(2000)),
        .event = event,
        .event_type = krbn::event_type::key_down,
        .event_integer_value = krbn::event_integer_value::value_t(1),
        .original_event = event,
        .state = krbn::event_queue::state::original,
    });

    expect(connector.min_input_event_time_stamp() == krbn::absolute_time_point(3000));

    // ----------------------------------------
    manipulator_managers.clear();
  };

  "deferred output session callback uses the current manipulators"_test = [] {
    auto manager = std::make_shared<krbn::manipulator::manipulator_manager>();
    auto callback = manager->make_post_deferred_modifier_key_up_events_callback();
    auto count = std::make_shared<std::size_t>(0);

    auto event = krbn::event_queue::event(krbn::momentary_switch_event(
        pqrs::hid::usage_page::keyboard_or_keypad,
        pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
    krbn::event_queue::entry entry({
        .device_id = krbn::device_id(1),
        .event_time_stamp = krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
        .event = event,
        .event_type = krbn::event_type::key_down,
        .original_event = event,
        .state = krbn::event_queue::state::original,
    });
    krbn::event_queue::queue queue;

    callback(entry, queue);
    expect(*count == 0_i);

    manager->push_back_manipulator(
        std::make_shared<deferred_modifier_key_up_events_test_manipulator>(count));

    callback(entry, queue);
    expect(*count == 1_i);

    manager->invalidate_manipulators();
    callback(entry, queue);
    expect(*count == 1_i);

    manager = nullptr;
    callback(entry, queue);
    expect(*count == 1_i);
  };

  "needs_virtual_hid_pointing"_test = [] {
    for (const auto& file_name : {
             std::string("json/needs_virtual_hid_pointing_test1.json"),
             std::string("json/needs_virtual_hid_pointing_test2.json"),
             std::string("json/needs_virtual_hid_pointing_test3.json"),
             std::string("json/needs_virtual_hid_pointing_test4.json"),
             std::string("json/needs_virtual_hid_pointing_test5.json"),
             std::string("json/needs_virtual_hid_pointing_test6.json"),
         }) {
      auto json = krbn::unit_testing::json_helper::load_jsonc(file_name);
      auto manager = std::make_shared<krbn::manipulator::manipulator_manager>();
      for (const auto& j : json) {
        auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
        auto m = krbn::manipulator::manipulator_factory::make_manipulator(j,
                                                                          parameters);
        manager->push_back_manipulator(m);
      }

      if (file_name == "json/needs_virtual_hid_pointing_test1.json") {
        expect(!manager->needs_virtual_hid_pointing());
      }
      if (file_name == "json/needs_virtual_hid_pointing_test2.json" ||
          file_name == "json/needs_virtual_hid_pointing_test3.json" ||
          file_name == "json/needs_virtual_hid_pointing_test4.json" ||
          file_name == "json/needs_virtual_hid_pointing_test5.json" ||
          file_name == "json/needs_virtual_hid_pointing_test6.json") {
        expect(manager->needs_virtual_hid_pointing());
      }

      manager = nullptr;
    }
  };

  "mouse_motion_and_wheel_to_key.needs_virtual_hid_pointing"_test = [] {
    for (const auto& test : krbn::unit_testing::json_helper::load_jsonc("json/mouse_motion_and_wheel_to_key_needs_virtual_hid_pointing.jsonc")) {
      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      auto m = krbn::manipulator::manipulator_factory::make_manipulator(test.at("input"), parameters);
      krbn::manipulator::manipulator_manager manager;
      manager.push_back_manipulator(m);
      expect(manager.needs_virtual_hid_pointing() == test.at("expected").get<bool>()) << test;
    }
  };

  "uses_caps_lock_led_override"_test = [] {
    for (const auto& test : krbn::unit_testing::json_helper::load_jsonc("json/uses_caps_lock_led_override.jsonc")) {
      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      auto m = krbn::manipulator::manipulator_factory::make_manipulator(test.at("input"), parameters);
      krbn::manipulator::manipulator_manager manager;
      manager.push_back_manipulator(m);

      expect(manager.uses_caps_lock_led_override() == test.at("expected").get<bool>()) << test;

      manager.invalidate_manipulators();
      expect(!manager.uses_caps_lock_led_override());
    }
  };
}
