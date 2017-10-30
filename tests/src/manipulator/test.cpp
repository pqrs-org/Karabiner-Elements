#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "../share/manipulator_helper.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE->emplace_back_event(krbn::device_id(DEVICE_ID),                                \
                            TIME_STAMP,                                                \
                            EVENT,                                                     \
                            krbn::event_type::EVENT_TYPE,                              \
                            ORIGINAL_EVENT);

#define ENQUEUE_LAZY_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE->emplace_back_event(krbn::device_id(DEVICE_ID),                                     \
                            TIME_STAMP,                                                     \
                            EVENT,                                                          \
                            krbn::event_type::EVENT_TYPE,                                   \
                            ORIGINAL_EVENT,                                                 \
                            true);

#define PUSH_BACK_QUEUED_EVENT(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  VECTOR.push_back(krbn::event_queue::queued_event(krbn::device_id(DEVICE_ID),                   \
                                                   TIME_STAMP,                                   \
                                                   EVENT,                                        \
                                                   krbn::event_type::EVENT_TYPE,                 \
                                                   ORIGINAL_EVENT))

#define PUSH_BACK_LAZY_QUEUED_EVENT(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  VECTOR.push_back(krbn::event_queue::queued_event(krbn::device_id(DEVICE_ID),                        \
                                                   TIME_STAMP,                                        \
                                                   EVENT,                                             \
                                                   krbn::event_type::EVENT_TYPE,                      \
                                                   ORIGINAL_EVENT,                                    \
                                                   true))

namespace {
krbn::event_queue::queued_event::event fn_event(krbn::key_code::fn);
krbn::event_queue::queued_event::event left_control_event(krbn::key_code::left_control);
krbn::event_queue::queued_event::event left_shift_event(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event right_shift_event(krbn::key_code::right_shift);
krbn::event_queue::queued_event::event spacebar_event(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event tab_event(krbn::key_code::tab);

auto device_ungrabbed_event = krbn::event_queue::queued_event::event::make_device_ungrabbed_event();
} // namespace

using krbn::manipulator::details::event_definition;
using krbn::manipulator::details::from_event_definition;
using krbn::manipulator::details::to_event_definition;

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
            "from", {
                        {
                            "key_code", "escape",
                        },
                        {
                            "modifiers", {
                                             {"mandatory", {
                                                               "left_shift", "left_option",
                                                           }},
                                             {"optional", {
                                                              "any",
                                                          }},
                                         },
                        },
                    },
        },
        {
            "to", {
                      {
                          {
                              "pointing_button", "button1",
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
    REQUIRE(basic->get_from().get_type() == event_definition::type::key_code);
    REQUIRE(basic->get_from().get_key_code() == krbn::key_code::escape);
    REQUIRE(basic->get_from().get_pointing_button() == boost::none);
    REQUIRE(basic->get_from().get_mandatory_modifiers() == std::unordered_set<event_definition::modifier>({
                                                               event_definition::modifier::left_shift,
                                                               event_definition::modifier::left_option,
                                                           }));
    REQUIRE(basic->get_from().get_optional_modifiers() == std::unordered_set<event_definition::modifier>({
                                                              event_definition::modifier::any,
                                                          }));
    REQUIRE(basic->get_to().size() == 1);
    REQUIRE(basic->get_to()[0].get_type() == event_definition::type::pointing_button);
    REQUIRE(basic->get_to()[0].get_key_code() == boost::none);
    REQUIRE(basic->get_to()[0].get_pointing_button() == krbn::pointing_button::button1);
    REQUIRE(basic->get_to()[0].get_modifiers() == std::unordered_set<event_definition::modifier>());
  }
}

TEST_CASE("manipulator.manipulator_manager") {
  krbn::unit_testing::manipulator_helper::run_tests(nlohmann::json::parse(std::ifstream("json/manipulator_manager/tests.json")));

  {
    // ----------------------------------------
    // manipulator_manager (invalidate)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

      REQUIRE(manipulator_manager.get_manipulators_size() == 1);
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::tab, {}, {}),
                                                                             to_event_definition(krbn::key_code::escape, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

      REQUIRE(manipulator_manager.get_manipulators_size() == 2);
    }

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);

    // ----------------------------------------
    // test

    connector.manipulate();
    connector.invalidate_manipulators();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, tab_event, key_down, spacebar_event);

      REQUIRE(input_event_queue->get_events().empty());
      REQUIRE(output_event_queue->get_events() == expected);
    }

    REQUIRE(manipulator_manager.get_manipulators_size() == 1);

    // push key_up (A remaining manipulator will be removed in the next `manipulate`)

    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);

    connector.manipulate();

    REQUIRE(manipulator_manager.get_manipulators_size() == 0);
  }

  {
    // ----------------------------------------
    // manipulator_manager (with modifiers)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar,
                                                                                                   {
                                                                                                       event_definition::modifier::fn,
                                                                                                   },
                                                                                                   {
                                                                                                       event_definition::modifier::any,
                                                                                                   }),
                                                                             to_event_definition(krbn::key_code::tab,
                                                                                                 {
                                                                                                     event_definition::modifier::fn,
                                                                                                 }));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

      REQUIRE(manipulator_manager.get_manipulators_size() == 1);
    }

    auto input_event_queue = std::make_shared<krbn::event_queue>();
    auto output_event_queue = std::make_shared<krbn::event_queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 200, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, fn_event, key_down, fn_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 500, fn_event, key_up, fn_event);
    ENQUEUE_EVENT(input_event_queue, 1, 600, spacebar_event, key_up, spacebar_event);

    // ----------------------------------------
    // test

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, spacebar_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 200, spacebar_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 300, fn_event, key_down, fn_event);
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 400, fn_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 401, tab_event, key_down, spacebar_event);
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 402, fn_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 502, fn_event, key_up, fn_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 602, tab_event, key_up, spacebar_event);
    }
  }
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
        file_name == "json/needs_virtual_hid_pointing_test4.json") {
      REQUIRE(manager.needs_virtual_hid_pointing());
    }
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
