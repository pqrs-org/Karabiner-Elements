#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "thread_utility.hpp"

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
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 500);

    std::vector<krbn::event_queue::queued_event> expected;

    expected.emplace_back(krbn::device_id(1),
                          100,
                          krbn::event_queue::queued_event::event(krbn::key_code::tab),
                          krbn::event_type::key_down,
                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    expected.emplace_back(krbn::device_id(1),
                          200,
                          krbn::event_queue::queued_event::event(krbn::key_code::escape),
                          krbn::event_type::key_down,
                          krbn::event_queue::queued_event::event(krbn::key_code::escape));

    expected.emplace_back(krbn::device_id(1),
                          300,
                          krbn::event_queue::queued_event::event(krbn::key_code::tab),
                          krbn::event_type::key_up,
                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    expected.emplace_back(krbn::device_id(1),
                          400,
                          krbn::event_queue::queued_event::event(krbn::key_code::escape),
                          krbn::event_type::key_up,
                          krbn::event_queue::queued_event::event(krbn::key_code::escape));

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (multiple manipulators)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::escape));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(
                                                                                 krbn::key_code::tab,
                                                                                 std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                                                     krbn::manipulator::details::event_definition::modifier::any,
                                                                                 })),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::a));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::escape),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::left_shift));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::left_shift),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::escape));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         500,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         600,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::left_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         700,
                                         krbn::event_queue::queued_event::event(krbn::key_code::right_shift),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::right_shift));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         800,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         900,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::tab));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 1000);

    std::vector<krbn::event_queue::queued_event> expected({
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        100,
                                        krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::spacebar)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        200,
                                        krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::escape)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        300,
                                        krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                        krbn::event_type::key_up,
                                        krbn::event_queue::queued_event::event(krbn::key_code::spacebar)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        400,
                                        krbn::event_queue::queued_event::event(krbn::key_code::left_shift),
                                        krbn::event_type::key_up,
                                        krbn::event_queue::queued_event::event(krbn::key_code::escape)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        500,
                                        krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::left_shift)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        600,
                                        krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                        krbn::event_type::key_up,
                                        krbn::event_queue::queued_event::event(krbn::key_code::left_shift)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        700,
                                        krbn::event_queue::queued_event::event(krbn::key_code::right_shift),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::right_shift)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        800,
                                        krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::spacebar)),
        krbn::event_queue::queued_event(krbn::device_id(1),
                                        900,
                                        krbn::event_queue::queued_event::event(krbn::key_code::a),
                                        krbn::event_type::key_down,
                                        krbn::event_queue::queued_event::event(krbn::key_code::tab)),
    });

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (key_up)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 200);

    std::vector<krbn::event_queue::queued_event> expected;

    expected.emplace_back(krbn::device_id(1),
                          100,
                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                          krbn::event_type::key_up,
                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    REQUIRE(input_event_queue.get_events().empty());
    REQUIRE(output_event_queue.get_events() == expected);
  }

  {
    // ----------------------------------------
    // manipulator_manager (inactivate_by_device_id)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(2),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 400);

    manipulator_manager.inactivate_by_device_id(krbn::device_id(1), output_event_queue, 500);

    {
      std::vector<krbn::event_queue::queued_event> expected;

      expected.emplace_back(krbn::device_id(1),
                            100,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(2),
                            200,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(1),
                            300,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    manipulator_manager.inactivate_by_device_id(krbn::device_id(2), output_event_queue, 500);

    {
      std::vector<krbn::event_queue::queued_event> expected;

      expected.emplace_back(krbn::device_id(1),
                            100,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(2),
                            200,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(1),
                            300,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(2),
                            500,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }
  }

  {
    // ----------------------------------------
    // manipulator_manager (invalidate)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));

      REQUIRE(manipulator_manager.get_manipulators_size() == 1);
    }
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::tab),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::escape));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));

      REQUIRE(manipulator_manager.get_manipulators_size() == 2);
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 200);

    manipulator_manager.invalidate();

    {
      std::vector<krbn::event_queue::queued_event> expected;

      expected.emplace_back(krbn::device_id(1),
                            100,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      REQUIRE(input_event_queue.get_events().empty());
      REQUIRE(output_event_queue.get_events() == expected);
    }

    REQUIRE(manipulator_manager.get_manipulators_size() == 1);

    // push key_up (A remaining manipulator will be removed in the next `manipulate`)

    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 400);

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
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(event_definition(krbn::key_code::spacebar, from_modifiers),
                                                                             event_definition(krbn::key_code::tab, to_modifiers));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));

      REQUIRE(manipulator_manager.get_manipulators_size() == 1);
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         500,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         600,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 700);

    {
      std::vector<krbn::event_queue::queued_event> expected;

      expected.emplace_back(krbn::device_id(1),
                            100,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(1),
                            200,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(1),
                            300,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn));

      expected.emplace_back(krbn::device_id(1),
                            400,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                            true);

      expected.emplace_back(krbn::device_id(1),
                            401,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

      expected.emplace_back(krbn::device_id(1),
                            402,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn),
                            krbn::event_type::key_down,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                            true);

      expected.emplace_back(krbn::device_id(1),
                            502,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::fn));

      expected.emplace_back(krbn::device_id(1),
                            602,
                            krbn::event_queue::queued_event::event(krbn::key_code::tab),
                            krbn::event_type::key_up,
                            krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    }
  }

  {
    // ----------------------------------------
    // manipulator_manager (lazy)

    krbn::manipulator::manipulator_manager manipulator_manager;
    {
      auto manipulator = std::make_unique<krbn::manipulator::details::basic>(krbn::manipulator::details::event_definition(krbn::key_code::spacebar),
                                                                             krbn::manipulator::details::event_definition(krbn::key_code::tab));
      std::unique_ptr<krbn::manipulator::details::base> ptr = std::move(manipulator);
      manipulator_manager.push_back_manipulator(std::move(ptr));
    }

    // ----------------------------------------
    // event_queue

    krbn::event_queue input_event_queue;
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         100,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         200,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         krbn::event_type::key_down,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         true);
    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         300,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                         true);

    // ----------------------------------------
    // test

    krbn::event_queue output_event_queue;

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 700);

    {
      std::vector<krbn::event_queue::queued_event> expected({
          krbn::event_queue::queued_event(krbn::device_id(1),
                                          100,
                                          krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                          krbn::event_type::key_down,
                                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar)),
      });

      REQUIRE(input_event_queue.get_events().size() == 2);
      REQUIRE(output_event_queue.get_events() == expected);
    }

    // Push a non lazy event

    input_event_queue.emplace_back_event(krbn::device_id(1),
                                         400,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar),
                                         krbn::event_type::key_up,
                                         krbn::event_queue::queued_event::event(krbn::key_code::spacebar));

    output_event_queue.clear_events();

    manipulator_manager.manipulate(input_event_queue, output_event_queue, 500);

    {
      std::vector<krbn::event_queue::queued_event> expected({
          krbn::event_queue::queued_event(krbn::device_id(1),
                                          200,
                                          krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                          krbn::event_type::key_down,
                                          krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                          true),
          krbn::event_queue::queued_event(krbn::device_id(1),
                                          300,
                                          krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                          krbn::event_type::key_up,
                                          krbn::event_queue::queued_event::event(krbn::key_code::fn),
                                          true),
          krbn::event_queue::queued_event(krbn::device_id(1),
                                          400,
                                          krbn::event_queue::queued_event::event(krbn::key_code::tab),
                                          krbn::event_type::key_up,
                                          krbn::event_queue::queued_event::event(krbn::key_code::spacebar)),
      });

      REQUIRE(output_event_queue.get_events() == expected);
    }
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
