#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_factory.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulator.details.event_definition") {
  {
    nlohmann::json json;
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::none);
    REQUIRE(event_definition.get_modifiers().size() == 0);
  }
  {
    nlohmann::json json({
        {"key", "spacebar"},
        {
            "modifiers", {
                             "shift", "left_command", "any",
                         },
        },
    });
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::key);
    REQUIRE(*(event_definition.get_key_code()) == *(krbn::types::get_key_code("spacebar")));
    REQUIRE(!(event_definition.get_pointing_button()));
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                    krbn::manipulator::details::event_definition::modifier::shift,
                                                    krbn::manipulator::details::event_definition::modifier::left_command,
                                                    krbn::manipulator::details::event_definition::modifier::any,
                                                }));
  }
  {
    nlohmann::json json({
        {"key", "right_option"},
        {
            "modifiers", {
                             "shift", "left_command", "any",
                             // duplicated
                             "shift", "left_command", "any",
                         },
        },
    });
    krbn::manipulator::details::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::details::event_definition::type::key);
    REQUIRE(*(event_definition.get_key_code()) == *(krbn::types::get_key_code("right_option")));
    REQUIRE(!(event_definition.get_pointing_button()));
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                    krbn::manipulator::details::event_definition::modifier::right_option,
                                                    krbn::manipulator::details::event_definition::modifier::shift,
                                                    krbn::manipulator::details::event_definition::modifier::left_command,
                                                    krbn::manipulator::details::event_definition::modifier::any,
                                                }));
  }
  {
    nlohmann::json json({
        {"key", nlohmann::json::array()},
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
                            "key", "escape",
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
    REQUIRE(basic->get_from().get_type() == krbn::manipulator::details::event_definition::type::key);
    REQUIRE(*(basic->get_from().get_key_code()) == *(krbn::types::get_key_code("escape")));
    REQUIRE(!(basic->get_from().get_pointing_button()));
    REQUIRE(basic->get_from().get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>({
                                                     krbn::manipulator::details::event_definition::modifier::left_shift,
                                                     krbn::manipulator::details::event_definition::modifier::left_option,
                                                     krbn::manipulator::details::event_definition::modifier::any,
                                                 }));
    REQUIRE(basic->get_to().size() == 1);
    REQUIRE(basic->get_to()[0].get_type() == krbn::manipulator::details::event_definition::type::pointing_button);
    REQUIRE(!(basic->get_to()[0].get_key_code()));
    REQUIRE(*(basic->get_to()[0].get_pointing_button()) == *(krbn::types::get_pointing_button("button1")));
    REQUIRE(basic->get_to()[0].get_modifiers() == std::unordered_set<krbn::manipulator::details::event_definition::modifier>());
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
