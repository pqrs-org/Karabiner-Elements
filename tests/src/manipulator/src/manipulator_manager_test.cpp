#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"

TEST_CASE("manipulator.manipulator_manager") {
  auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();
  helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/manipulator_manager/tests.json"));

  helper = nullptr;
}

TEST_CASE("min_input_event_time_stamp") {
  std::vector<std::shared_ptr<krbn::event_queue::queue>> event_queues;
  event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue::queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue::queue>());

  krbn::manipulator::manipulator_managers_connector connector;

  std::vector<std::shared_ptr<krbn::manipulator::manipulator_manager>> manipulator_managers;
  for (size_t i = 0; i < event_queues.size() - 1; ++i) {
    manipulator_managers.push_back(std::make_shared<krbn::manipulator::manipulator_manager>());
  }

  for (size_t i = 0; i < manipulator_managers.size(); ++i) {
    connector.emplace_back_connection(manipulator_managers[i],
                                      event_queues[i],
                                      event_queues[i + 1]);
  }

  REQUIRE(!connector.min_input_event_time_stamp());

  // ----------------------------------------

  event_queues[2]->emplace_back_entry(krbn::device_id(1),
                                      krbn::event_queue::event_time_stamp(krbn::absolute_time_point(5000)),
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_queue::state::original);

  REQUIRE(connector.min_input_event_time_stamp() == krbn::absolute_time_point(5000));

  // ----------------------------------------

  event_queues[0]->emplace_back_entry(krbn::device_id(1),
                                      krbn::event_queue::event_time_stamp(krbn::absolute_time_point(4000)),
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_queue::state::original);

  REQUIRE(connector.min_input_event_time_stamp() == krbn::absolute_time_point(4000));

  // ----------------------------------------
  // min_input_event_time_stamp uses only the front entry.

  event_queues[3]->emplace_back_entry(krbn::device_id(1),
                                      krbn::event_queue::event_time_stamp(krbn::absolute_time_point(3000)),
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_queue::state::original);
  event_queues[0]->emplace_back_entry(krbn::device_id(1),
                                      krbn::event_queue::event_time_stamp(krbn::absolute_time_point(2000)),
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                      krbn::event_queue::state::original);

  REQUIRE(connector.min_input_event_time_stamp() == krbn::absolute_time_point(3000));

  // ----------------------------------------
  manipulator_managers.clear();
}

TEST_CASE("needs_virtual_hid_pointing") {
  for (const auto& file_name : {
           std::string("json/needs_virtual_hid_pointing_test1.json"),
           std::string("json/needs_virtual_hid_pointing_test2.json"),
           std::string("json/needs_virtual_hid_pointing_test3.json"),
           std::string("json/needs_virtual_hid_pointing_test4.json"),
           std::string("json/needs_virtual_hid_pointing_test5.json"),
       }) {
    auto json = krbn::unit_testing::json_helper::load_jsonc(file_name);
    auto manager = std::make_shared<krbn::manipulator::manipulator_manager>();
    for (const auto& j : json) {
      krbn::core_configuration::details::complex_modifications_parameters parameters;
      auto m = krbn::manipulator::manipulator_factory::make_manipulator(j,
                                                                        parameters);
      manager->push_back_manipulator(m);
    }

    if (file_name == "json/needs_virtual_hid_pointing_test1.json") {
      REQUIRE(!manager->needs_virtual_hid_pointing());
    }
    if (file_name == "json/needs_virtual_hid_pointing_test2.json" ||
        file_name == "json/needs_virtual_hid_pointing_test3.json" ||
        file_name == "json/needs_virtual_hid_pointing_test4.json" ||
        file_name == "json/needs_virtual_hid_pointing_test5.json") {
      REQUIRE(manager->needs_virtual_hid_pointing());
    }

    manager = nullptr;
  }
}
