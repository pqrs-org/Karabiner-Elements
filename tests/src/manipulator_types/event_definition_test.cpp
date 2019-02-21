#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

TEST_CASE("to_event_definition") {
  {
    nlohmann::json json({
        {"key_code", "spacebar"},
        {"modifiers", {
                          "shift",
                          "left_command",
                      }},
    });
    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definition().get_key_code() == krbn::key_code::spacebar);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::modifier_definition::modifier>({
                                                    krbn::manipulator::modifier_definition::modifier::shift,
                                                    krbn::manipulator::modifier_definition::modifier::left_command,
                                                }));
    REQUIRE(event_definition.get_event_definition().to_event() == krbn::event_queue::event(krbn::key_code::spacebar));
    REQUIRE(event_definition.make_modifier_events() == std::vector<krbn::event_queue::event>({
                                                           krbn::event_queue::event(krbn::key_code::left_command),
                                                           krbn::event_queue::event(krbn::key_code::left_shift),
                                                       }));
  }
  {
    nlohmann::json json({
        {"key_code", "right_option"},
        {"modifiers", {
                          "shift",
                          "left_command",
                          // duplicated
                          "shift",
                          "left_command",
                      }},
    });
    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definition().get_key_code() == krbn::key_code::right_option);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::modifier_definition::modifier>({
                                                    krbn::manipulator::modifier_definition::modifier::shift,
                                                    krbn::manipulator::modifier_definition::modifier::left_command,
                                                }));
    REQUIRE(event_definition.make_modifier_events() == std::vector<krbn::event_queue::event>({
                                                           krbn::event_queue::event(krbn::key_code::left_command),
                                                           krbn::event_queue::event(krbn::key_code::left_shift),
                                                       }));
  }

  // modifiers error

  {
    nlohmann::json json({
        {"key_code", "a"},
        {"modifiers", {
                          "dummy",
                      }},
    });
    REQUIRE_THROWS_AS(
        krbn::manipulator::to_event_definition(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::manipulator::to_event_definition(json),
        "unknown modifier: `dummy`");
  }

  {
    std::string shell_command = "open /Applications/Safari.app";
    nlohmann::json json({
        {"shell_command", shell_command},
    });
    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::shell_command);
    REQUIRE(event_definition.get_event_definition().get_key_code() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == shell_command);
    REQUIRE(event_definition.get_event_definition().get_input_source_specifiers() == std::nullopt);
  }
  // select_input_source
  {
    pqrs::osx::input_source_selector::specifier s;
    s.set_input_source_id("^com\\.apple\\.keylayout\\.US$");

    nlohmann::json json;
    json["select_input_source"]["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::select_input_source);
    REQUIRE(event_definition.get_event_definition().get_key_code() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_input_source_specifiers() == std::vector<pqrs::osx::input_source_selector::specifier>({s}));
  }
  // select_input_source (array)
  {
    pqrs::osx::input_source_selector::specifier s1;
    s1.set_input_source_id("^com\\.apple\\.keylayout\\.US$");
    pqrs::osx::input_source_selector::specifier s2;
    s2.set_language("^en$");
    std::vector<pqrs::osx::input_source_selector::specifier> specifiers{s1, s2};

    nlohmann::json json;
    json["select_input_source"] = nlohmann::json::array();
    json["select_input_source"].push_back(nlohmann::json::object());
    json["select_input_source"].back()["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";
    json["select_input_source"].push_back(nlohmann::json::object());
    json["select_input_source"].back()["language"] = "^en$";

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::select_input_source);
    REQUIRE(event_definition.get_event_definition().get_key_code() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == std::nullopt);
    REQUIRE(event_definition.get_event_definition().get_input_source_specifiers() == specifiers);
  }
  // lazy
  {
    auto json = nlohmann::json::object({
        {"key_code", "left_shift"},
        {"lazy", true},
    });

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_lazy() == true);
  }
  // lazy
  {
    auto json = nlohmann::json::object({
        {"key_code", "a"},
        {"repeat", false},
    });

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_repeat() == false);
  }
}
