#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

using krbn::manipulator::details::event_definition;
using krbn::manipulator::details::from_event_definition;
using krbn::manipulator::details::modifier_definition;
using krbn::manipulator::details::to_event_definition;

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("manipulator.manipulator_factory") {
  {
    nlohmann::json json;
    krbn::core_configuration::profile::complex_modifications::parameters parameters;
    auto manipulator = krbn::manipulator::manipulator_factory::make_manipulator(json, parameters);
    REQUIRE(dynamic_cast<krbn::manipulator::details::nop*>(manipulator.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::basic*>(manipulator.get()) == nullptr);
    REQUIRE(manipulator->get_valid() == true);
    REQUIRE(manipulator->active() == false);
  }
  {
    nlohmann::json json({
        {"type", "basic"},
        {
            "from",
            {
                {
                    "key_code",
                    "escape",
                },
                {
                    "modifiers",
                    {
                        {"mandatory", {
                                          "left_shift",
                                          "left_option",
                                      }},
                        {"optional", {
                                         "any",
                                     }},
                    },
                },
            },
        },
        {
            "to",
            {
                {
                    {
                        "pointing_button",
                        "button1",
                    },
                },
            },
        },
    });
    krbn::core_configuration::profile::complex_modifications::parameters parameters;
    auto manipulator = krbn::manipulator::manipulator_factory::make_manipulator(json, parameters);
    REQUIRE(dynamic_cast<krbn::manipulator::details::basic*>(manipulator.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::nop*>(manipulator.get()) == nullptr);
    REQUIRE(manipulator->get_valid() == true);
    REQUIRE(manipulator->active() == false);

    auto basic = dynamic_cast<krbn::manipulator::details::basic*>(manipulator.get());
    REQUIRE(basic->get_from().get_event_definitions().size() == 1);
    REQUIRE(basic->get_from().get_event_definitions().front().get_type() == event_definition::type::key_code);
    REQUIRE(basic->get_from().get_event_definitions().front().get_key_code() == krbn::key_code::escape);
    REQUIRE(basic->get_from().get_event_definitions().front().get_pointing_button() == boost::none);
    REQUIRE(basic->get_from().get_mandatory_modifiers() == std::unordered_set<modifier_definition::modifier>({
                                                               modifier_definition::modifier::left_shift,
                                                               modifier_definition::modifier::left_option,
                                                           }));
    REQUIRE(basic->get_from().get_optional_modifiers() == std::unordered_set<modifier_definition::modifier>({
                                                              modifier_definition::modifier::any,
                                                          }));
    REQUIRE(basic->get_to().size() == 1);
    REQUIRE(basic->get_to()[0].get_event_definition().get_type() == event_definition::type::pointing_button);
    REQUIRE(basic->get_to()[0].get_event_definition().get_key_code() == boost::none);
    REQUIRE(basic->get_to()[0].get_event_definition().get_pointing_button() == krbn::pointing_button::button1);
    REQUIRE(basic->get_to()[0].get_modifiers() == std::unordered_set<modifier_definition::modifier>());
  }
}

TEST_CASE("manipulator.manipulator_manager") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/manipulator_manager/tests.json")));
}

TEST_CASE("min_input_event_time_stamp") {
  std::vector<std::shared_ptr<krbn::event_queue>> event_queues;
  event_queues.push_back(std::make_shared<krbn::event_queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue>());
  event_queues.push_back(std::make_shared<krbn::event_queue>());

  krbn::manipulator::manipulator_managers_connector connector;

  std::vector<krbn::manipulator::manipulator_manager> manipulator_managers(event_queues.size() - 1);

  for (size_t i = 0; i < manipulator_managers.size(); ++i) {
    connector.emplace_back_connection(manipulator_managers[i],
                                      event_queues[i],
                                      event_queues[i + 1]);
  }

  REQUIRE(!connector.min_input_event_time_stamp());

  // ----------------------------------------

  event_queues[2]->emplace_back_event(krbn::device_id(1),
                                      krbn::event_queue::queued_event::event_time_stamp(5000),
                                      krbn::event_queue::queued_event::event(krbn::key_code::a),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::queued_event::event(krbn::key_code::a));

  REQUIRE(connector.min_input_event_time_stamp() == 5000ull);

  // ----------------------------------------

  event_queues[0]->emplace_back_event(krbn::device_id(1),
                                      krbn::event_queue::queued_event::event_time_stamp(4000),
                                      krbn::event_queue::queued_event::event(krbn::key_code::a),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::queued_event::event(krbn::key_code::a));

  REQUIRE(connector.min_input_event_time_stamp() == 4000ull);

  // ----------------------------------------
  // min_input_event_time_stamp uses only the front entry.

  event_queues[3]->emplace_back_event(krbn::device_id(1),
                                      krbn::event_queue::queued_event::event_time_stamp(3000),
                                      krbn::event_queue::queued_event::event(krbn::key_code::a),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::queued_event::event(krbn::key_code::a));
  event_queues[0]->emplace_back_event(krbn::device_id(1),
                                      krbn::event_queue::queued_event::event_time_stamp(2000),
                                      krbn::event_queue::queued_event::event(krbn::key_code::a),
                                      krbn::event_type::key_down,
                                      krbn::event_queue::queued_event::event(krbn::key_code::a));

  REQUIRE(connector.min_input_event_time_stamp() == 3000ull);
}

TEST_CASE("manipulator_timer") {
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries().empty());

  std::vector<krbn::manipulator::manipulator_timer::timer_id> timer_ids;
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(1234));
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(1234));
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(5678));
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(5678));
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(2345));
  timer_ids.push_back(krbn::manipulator::manipulator_timer::get_instance().add_entry(2345));

  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries().size() == 6);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[0].get_when() == 1234);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[1].get_when() == 1234);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[2].get_when() == 2345);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[3].get_when() == 2345);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[4].get_when() == 5678);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[5].get_when() == 5678);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[0].get_timer_id() == timer_ids[0]);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[1].get_timer_id() == timer_ids[1]);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[2].get_timer_id() == timer_ids[4]);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[3].get_timer_id() == timer_ids[5]);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[4].get_timer_id() == timer_ids[2]);
  REQUIRE(krbn::manipulator::manipulator_timer::get_instance().get_entries()[5].get_timer_id() == timer_ids[3]);
}

TEST_CASE("needs_virtual_hid_pointing") {
  for (const auto& file_name : {
           std::string("json/needs_virtual_hid_pointing_test1.json"),
           std::string("json/needs_virtual_hid_pointing_test2.json"),
           std::string("json/needs_virtual_hid_pointing_test3.json"),
           std::string("json/needs_virtual_hid_pointing_test4.json"),
           std::string("json/needs_virtual_hid_pointing_test5.json"),
       }) {
    std::ifstream json_file(file_name);
    auto json = nlohmann::json::parse(json_file);
    krbn::manipulator::manipulator_manager manager;
    for (const auto& j : json) {
      krbn::core_configuration::profile::complex_modifications::parameters parameters;
      auto m = krbn::manipulator::manipulator_factory::make_manipulator(j, parameters);
      manager.push_back_manipulator(m);
    }

    if (file_name == "json/needs_virtual_hid_pointing_test1.json") {
      REQUIRE(!manager.needs_virtual_hid_pointing());
    }
    if (file_name == "json/needs_virtual_hid_pointing_test2.json" ||
        file_name == "json/needs_virtual_hid_pointing_test3.json" ||
        file_name == "json/needs_virtual_hid_pointing_test4.json" ||
        file_name == "json/needs_virtual_hid_pointing_test5.json") {
      REQUIRE(manager.needs_virtual_hid_pointing());
    }
  }
}
