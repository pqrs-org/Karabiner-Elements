#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

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
krbn::event_queue::queued_event::event fn_event(krbn::key_code::fn);
krbn::event_queue::queued_event::event left_control_event(krbn::key_code::left_control);
krbn::event_queue::queued_event::event left_shift_event(krbn::key_code::left_shift);
krbn::event_queue::queued_event::event page_up_event(krbn::key_code::page_up);
krbn::event_queue::queued_event::event right_shift_event(krbn::key_code::right_shift);
krbn::event_queue::queued_event::event spacebar_event(krbn::key_code::spacebar);
krbn::event_queue::queued_event::event tab_event(krbn::key_code::tab);
krbn::event_queue::queued_event::event up_arrow_event(krbn::key_code::up_arrow);

krbn::event_queue::queued_event::event device_ungrabbed_event(krbn::event_queue::queued_event::event::type::device_ungrabbed, 1);
} // namespace

TEST_CASE("manipulator.details.event_definition") {
  {
    nlohmann::json json;
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::none);
    REQUIRE(event_definition.get_modifiers().size() == 0);
    REQUIRE(!(event_definition.to_event()));
  }
  {
    nlohmann::json json({
        {"key_code", "spacebar"},
        {
            "modifiers", {
                             "shift", "left_command", "any",
                         },
        },
    });
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::key_code);
    REQUIRE(*(event_definition.get_key_code()) == krbn::key_code::spacebar);
    REQUIRE(!(event_definition.get_pointing_button()));
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                    krbn::manipulator::details::event_definition::modifier::shift,
                                                    krbn::manipulator::details::event_definition::modifier::left_command,
                                                    krbn::manipulator::details::event_definition::modifier::any,
                                                }));
    REQUIRE(*(event_definition.to_event()) == krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
  }
  {
    nlohmann::json json({
        {"key_code", "right_option"},
        {
            "modifiers", {
                             "shift", "left_command", "any",
                             // duplicated
                             "shift", "left_command", "any",
                         },
        },
    });
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::key_code);
    REQUIRE(*(event_definition.get_key_code()) == krbn::key_code::right_option);
    REQUIRE(!(event_definition.get_pointing_button()));
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                    krbn::manipulator::details::event_definition::modifier::shift,
                                                    krbn::manipulator::details::event_definition::modifier::left_command,
                                                    krbn::manipulator::details::event_definition::modifier::any,
                                                }));
  }
  {
    nlohmann::json json({
        {"key_code", nlohmann::json::array()},
        {
            "modifiers", "dummy",
        },
    });
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::none);
    REQUIRE(!(event_definition.get_key_code()));
    REQUIRE(!(event_definition.get_pointing_button()));
    REQUIRE(event_definition.get_modifiers().size() == 0);
  }
}

TEST_CASE("manipulator.manipulator_factory") {
  {
    nlohmann::json json;
    auto manipulator = krbn::manipulator::manipulator_factory::make_manipulator(json);
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
                                             "left_shift", "left_option", "any",
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
    auto manipulator = krbn::manipulator::manipulator_factory::make_manipulator(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::basic*>(manipulator.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::nop*>(manipulator.get()) == nullptr);
    REQUIRE(manipulator->get_valid() == true);
    REQUIRE(manipulator->active() == false);

    auto basic = dynamic_cast<krbn::manipulator::details::basic*>(manipulator.get());
    REQUIRE(basic->get_from().get_type() == krbn::manipulator::details::event_definition::type::key_code);
    REQUIRE(*(basic->get_from().get_key_code()) == krbn::key_code::escape);
    REQUIRE(!(basic->get_from().get_pointing_button()));
    REQUIRE(basic->get_from().get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                     krbn::manipulator::details::event_definition::modifier::left_shift,
                                                     krbn::manipulator::details::event_definition::modifier::left_option,
                                                     krbn::manipulator::details::event_definition::modifier::any,
                                                 }));
    REQUIRE(basic->get_to().size() == 1);
    REQUIRE(basic->get_to()[0].get_type() == krbn::manipulator::details::event_definition::type::pointing_button);
    REQUIRE(!(basic->get_to()[0].get_key_code()));
    REQUIRE(*(basic->get_to()[0].get_pointing_button()) == krbn::pointing_button::button1);
    REQUIRE(basic->get_to()[0].get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>());
  }
}

TEST_CASE("manipulator.manipulator_manager") {
  using krbn::manipulator::details::event_definition;

  {
    // ----------------------------------------
    // manipulator_manager

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
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

    connector.manipulate(500);

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
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::escape));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(
                                                                                 krbn::key_code::tab,
                                                                                 std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                                                     krbn::manipulator::details::event_definition::modifier::any,
                                                                                 })),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::a));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::escape),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::left_shift));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::left_shift),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
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

    // ----------------------------------------
    // test

    connector.manipulate(1000);

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

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (key_up)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
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

    connector.manipulate(200);

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
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      manipulator_manager1.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::manipulator::manipulator_manager manipulator_manager2;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::tab),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::escape));
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
                                      middle_event_queue,
                                      output_event_queue);

    // ----------------------------------------
    // event_queue

    ENQUEUE_EVENT(input_event_queue, 1, 100, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 2, 200, spacebar_event, key_down, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, device_ungrabbed_event, key_down, device_ungrabbed_event);

    // ----------------------------------------
    // test

    // device_ungrabbed for device_id 1

    connector.manipulate(500);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 200, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 300, escape_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 400, device_ungrabbed_event, key_down, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 2, 600, spacebar_event, key_up, spacebar_event);

    connector.manipulate(500);

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
    ENQUEUE_EVENT(input_event_queue, 2, 400, device_ungrabbed_event, key_down, device_ungrabbed_event);

    connector.manipulate(500);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 200, escape_event, key_down, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 300, escape_event, key_up, spacebar_event);
      PUSH_BACK_QUEUED_EVENT(expected, 2, 400, device_ungrabbed_event, key_down, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 2, 600, spacebar_event, key_up, spacebar_event);

    connector.manipulate(500);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 2, 600, spacebar_event, key_up, spacebar_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    // device_ungrabbed for device_id 3

    output_event_queue.clear_events();

    ENQUEUE_EVENT(input_event_queue, 3, 100, device_ungrabbed_event, key_down, device_ungrabbed_event);

    connector.manipulate(600);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 3, 100, device_ungrabbed_event, key_down, device_ungrabbed_event);

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }
  }

  {
    // ----------------------------------------
    // manipulator_manager (invalidate)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));

      REQUIRE(manipulator_manager.get_manipulators_size() == 1);
    }
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::tab),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::escape));
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

    connector.manipulate(200);
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

    connector.manipulate(400);

    REQUIRE(manipulator_manager.get_manipulators_size() == 0);
  }

  {
    // ----------------------------------------
    // manipulator_manager (with modifiers)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      std::unordered_set<event_definition::modifier> from_modifiers({
          event_definition::modifier::fn,
          event_definition::modifier::any,
      });
      std::unordered_set<event_definition::modifier> to_modifiers({
          event_definition::modifier::fn,
      });
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(event_definition(krbn::key_code::spacebar, from_modifiers),
                                                                             event_definition(krbn::key_code::tab, to_modifiers));
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

    connector.manipulate(700);

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

  {
    // ----------------------------------------
    // manipulator_manager (lazy)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
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
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 200, fn_event, key_down, fn_event);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 300, fn_event, key_up, fn_event);

    // ----------------------------------------
    // test

    connector.manipulate(700);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, tab_event, key_down, spacebar_event);

      REQUIRE(input_event_queue.get_events().size() == 2);
      REQUIRE(output_event_queue.get_events() == expected);
    }

    // Push a non lazy event

    ENQUEUE_EVENT(input_event_queue, 1, 400, spacebar_event, key_up, spacebar_event);

    output_event_queue.clear_events();

    connector.manipulate(500);

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 200, fn_event, key_down, fn_event);
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 300, fn_event, key_up, fn_event);
      PUSH_BACK_QUEUED_EVENT(expected, 1, 400, tab_event, key_up, spacebar_event);

      REQUIRE(output_event_queue.get_events() == expected);
    }
  }
}

TEST_CASE("manipulator.details.collapse_lazy_events") {
  {
    // ----------------------------------------
    // Keep not-lazy events

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::collapse_lazy_events>();
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    ENQUEUE_EVENT(input_event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, 200, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, left_control_event, key_up, left_control_event);
    ENQUEUE_EVENT(input_event_queue, 1, 500, spacebar_event, key_up, spacebar_event);

    connector.manipulate(600);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 200, left_control_event, key_down, left_control_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, left_shift_event, key_up, left_shift_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 400, left_control_event, key_up, left_control_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 500, spacebar_event, key_up, spacebar_event);

    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // Collapse lazy events
    //   * left_shift:   key_down,key_up
    //   * left_control: key_up,key_down

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::collapse_lazy_events>();
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 200, left_control_event, key_up, left_control_event);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 300, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(input_event_queue, 1, 500, spacebar_event, key_up, spacebar_event);

    connector.manipulate(500);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 500, spacebar_event, key_up, spacebar_event);

    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // Keep lazy events if corresponded event is not found.

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::collapse_lazy_events>();
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 200, left_control_event, key_down, left_control_event);
    ENQUEUE_EVENT(input_event_queue, 1, 300, spacebar_event, key_up, spacebar_event);

    connector.manipulate(500);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 100, left_shift_event, key_down, left_shift_event);
    PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 200, left_control_event, key_down, left_control_event);
    PUSH_BACK_QUEUED_EVENT(expected, 1, 300, spacebar_event, key_up, spacebar_event);

    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // Collapse lazy events (split key_down and key_up)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::collapse_lazy_events>();
      manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(manipulator_manager,
                                      input_event_queue,
                                      output_event_queue);

    // First

    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 100, left_shift_event, key_down, left_shift_event);
    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 200, left_control_event, key_down, left_control_event);

    connector.manipulate(300);

    REQUIRE(output_event_queue.get_events().empty());

    // Second

    ENQUEUE_LAZY_EVENT(input_event_queue, 1, 300, left_shift_event, key_up, left_shift_event);
    ENQUEUE_EVENT(input_event_queue, 1, 400, left_control_event, key_up, left_control_event);
    ENQUEUE_EVENT(input_event_queue, 1, 500, spacebar_event, key_up, spacebar_event);

    connector.manipulate(600);

    std::vector<krbn::event_queue::queued_event> expected;
    PUSH_BACK_QUEUED_EVENT(expected, 1, 500, spacebar_event, key_up, spacebar_event);

    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // Collapse lazy events (actual example)

    krbn::manipulator::manipulator_manager fn_manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(
                                                                                 krbn::key_code::up_arrow,
                                                                                 std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                                                     krbn::manipulator::details::event_definition::modifier::fn,
                                                                                     krbn::manipulator::details::event_definition::modifier::any,
                                                                                 })),
                                                                             krbn::manipulator::details::event_definition(
                                                                                 krbn::key_code::page_up,
                                                                                 std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                                                     krbn::manipulator::details::event_definition::modifier::fn,
                                                                                 })));
      fn_manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::manipulator::manipulator_manager lazy_manipulator_manager;
    {
      auto manipulator = std::make_shared<krbn::manipulator::details::collapse_lazy_events>();
      lazy_manipulator_manager.push_back_manipulator(std::shared_ptr<krbn::manipulator::details::base>(manipulator));
    }

    krbn::event_queue input_event_queue;
    krbn::event_queue middle_event_queue;
    krbn::event_queue output_event_queue;

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(fn_manipulator_manager,
                                      input_event_queue,
                                      middle_event_queue);
    connector.emplace_back_connection(lazy_manipulator_manager,
                                      middle_event_queue,
                                      output_event_queue);

    // fn key_down

    ENQUEUE_EVENT(input_event_queue, 1, 100, fn_event, key_down, fn_event);

    connector.manipulate(200);

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(middle_event_queue.get_events().empty());

    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 100, fn_event, key_down, fn_event);

      REQUIRE(output_event_queue.get_events() == expected);
      output_event_queue.erase_front_event();
      REQUIRE(output_event_queue.get_events().empty());
    }

    // up_arrow key_down

    ENQUEUE_EVENT(input_event_queue, 1, 300, up_arrow_event, key_down, up_arrow_event);

    connector.manipulate(400);

    REQUIRE(input_event_queue.get_events().empty());
    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 303, fn_event, key_up, up_arrow_event);
      PUSH_BACK_LAZY_QUEUED_EVENT(expected, 1, 304, fn_event, key_down, up_arrow_event);
      REQUIRE(middle_event_queue.get_events() == expected);
    }
    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 302, page_up_event, key_down, up_arrow_event);
      REQUIRE(output_event_queue.get_events() == expected);
      output_event_queue.erase_front_event();
      REQUIRE(output_event_queue.get_events().empty());
    }

    // up_arrow key_up

    ENQUEUE_EVENT(input_event_queue, 1, 500, up_arrow_event, key_up, up_arrow_event);

    connector.manipulate(600);

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(middle_event_queue.get_events().empty());
    {
      std::vector<krbn::event_queue::queued_event> expected;
      PUSH_BACK_QUEUED_EVENT(expected, 1, 504, page_up_event, key_up, up_arrow_event);
      REQUIRE(output_event_queue.get_events() == expected);
      output_event_queue.erase_front_event();
      REQUIRE(output_event_queue.get_events().empty());
    }
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
