#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/details/types.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulator.details.event_definition") {
  {
    nlohmann::json json;
    krbn::manipulator::detail::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::detail::event_definition::type::none);
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
    krbn::manipulator::detail::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::detail::event_definition::type::key);
    REQUIRE(event_definition.get_value() == "spacebar");
    REQUIRE(event_definition.get_modifiers() == std::vector<krbn::manipulator::detail::event_definition::modifier>({
                                                    krbn::manipulator::detail::event_definition::modifier::shift,
                                                    krbn::manipulator::detail::event_definition::modifier::left_command,
                                                    krbn::manipulator::detail::event_definition::modifier::any,
                                                }));
  }
  {
    nlohmann::json json({
        {"key", nlohmann::json::array()},
        {
            "modifiers", "dummy",
        },
    });
    krbn::manipulator::detail::event_definition event_definition(json);
    REQUIRE(event_definition.get_type() == krbn::manipulator::detail::event_definition::type::none);
    REQUIRE(event_definition.get_value() == "");
    REQUIRE(event_definition.get_modifiers().size() == 0);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
