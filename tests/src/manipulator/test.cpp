#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                 \
                           TIME_STAMP,                                                 \
                           EVENT,                                                      \
                           krbn::event_type::EVENT_TYPE,                               \
                           ORIGINAL_EVENT);

#define ENQUEUE_LAZY_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT) \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                      \
                           TIME_STAMP,                                                      \
                           EVENT,                                                           \
                           krbn::event_type::EVENT_TYPE,                                    \
                           ORIGINAL_EVENT,                                                  \
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
krbn::event_queue::queued_event::event a_event(krbn::key_code::a);
krbn::event_queue::queued_event::event escape_event(krbn::key_code::escape);
krbn::event_queue::queued_event::event f1_event(krbn::key_code::f1);
krbn::event_queue::queued_event::event fn_event(krbn::key_code::fn);
krbn::event_queue::queued_event::event left_control_event(krbn::key_code::left_control);
krbn::event_queue::queued_event::event left_shift_event(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event page_up_event(krbn::key_code::page_up);
krbn::event_queue::queued_event::event right_shift_event(krbn::key_code::right_shift);
krbn::event_queue::queued_event::event spacebar_event(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event tab_event(krbn::key_code::tab);
krbn::event_queue::queued_event::event up_arrow_event(krbn::key_code::up_arrow);

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
  {
    // ----------------------------------------
    // manipulator_manager

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 200, escape_event, key_down, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, escape_event, key_up, escape_event);

    // ----------------------------------------
    // test

    connector.manipulate();

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, tab_event, key_down, spacebar_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 200, escape_event, key_down, escape_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, tab_event, key_up, spacebar_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 400, escape_event, key_up, escape_event);

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (multiple manipulators)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      // spacebar -> tab
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      // spacebar -> escape
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::escape, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      // tab -> a
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::tab,
                                                                                                   {},
                                                                                                   {
                                                                                                       event_definition::modifier::any,
                                                                                                   }),
                                                                             to_event_definition(krbn::key_code::a, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      // escape -> left_shift
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::escape, {}, {}),
                                                                             to_event_definition(krbn::key_code::left_shift, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      // left_shift -> tab
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::left_shift, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      // f1 -> empty
      krbn::core_configuration::profile::complex_modifications::parameters parameters;
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(
          nlohmann::json({
              {"type", "basic"},
              {"from",
               {
                   {"key_code", "f1"},
                   {"modifiers", {
                                     {"mandatory", nlohmann::json::array()}, {"optional", {"any"}},
                                 }},
               }},
          }),
          parameters);
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);
    // ----------------------------------------
    // event_queue

    uint64_t time_stamp = 0;
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, escape_event, key_down, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, escape_event, key_up, escape_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, right_shift_event, key_down, right_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, tab_event, key_down, tab_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, f1_event, key_down, f1_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, f1_event, key_up, f1_event);
    ENQUEUE_EVENT(input_event_queue, 1, time_stamp += 100, tab_event, key_up, tab_event);

    // ----------------------------------------
    // test

    connector.manipulate();

    std::vector<krbn::event_queue::queued_event> expected;
    time_stamp = 0;
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, tab_event, key_down, spacebar_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, left_shift_event, key_down, escape_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, tab_event, key_up, spacebar_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, left_shift_event, key_up, escape_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, tab_event, key_down, left_shift_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, tab_event, key_up, left_shift_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, right_shift_event, key_down, right_shift_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, spacebar_event, key_down, spacebar_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, a_event, key_down, tab_event);
    time_stamp += 100;
    time_stamp += 100;
    PUSH_BACK_QUEUED_EVENT(expected, 1, time_stamp += 100, a_event, key_up, tab_event);

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (key_up)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_up, spacebar_event);

    // ----------------------------------------
    // test

    connector.manipulate();

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, spacebar_event, key_up, spacebar_event);

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (handle_device_ungrabbed_event)

    krbn::manipulator::manipulator_manager manipulator_manager1;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::spacebar, {}, {}),
                                                                             to_event_definition(krbn::key_code::tab, {}));
      manipulator_manager1.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::manipulator::manipulator_manager manipulator_manager2;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(from_event_definition(krbn::key_code::tab, {}, {}),
                                                                             to_event_definition(krbn::key_code::escape, {}));
      manipulator_manager2.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue middle_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager1,
                                      input_event_queue,
                                      middle_event_queue);
    connector.emplace_back_connection(manipulator_manager2,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 2, 200, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, device_ungrabbed_event, single, device_ungrabbed_event);

    // ----------------------------------------
    // test

    // device_ungrabbed for device_id 1

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 200, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 300, escape_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 400, device_ungrabbed_event, single, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 2, 600, spacebar_event, key_up, spacebar_event);

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 2, 600, escape_event, key_up, spacebar_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    output_event_queue.clear_events();

    // device_ungrabbed for device_id 2

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 2, 200, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 2, 400, device_ungrabbed_event, single, device_ungrabbed_event);

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 200, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 300, escape_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 400, device_ungrabbed_event, single, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 2, 600, spacebar_event, key_up, spacebar_event);

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 2, 600, spacebar_event, key_up, spacebar_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    // device_ungrabbed for device_id 3

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 3, 100, device_ungrabbed_event, single, device_ungrabbed_event);

    connector.manipulate();

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 3, 100, device_ungrabbed_event, single, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }
  }

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

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

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

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
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

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

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
