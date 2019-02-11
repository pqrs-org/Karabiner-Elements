#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

namespace {
void set_file_logger(const std::string& file_path) {
  unlink(file_path.c_str());
  auto l = spdlog::rotating_logger_mt(file_path, file_path, 256 * 1024, 3);
  l->set_pattern("[%l] %v");
  krbn::logger::set_logger(l);
}

void set_null_logger(void) {
  static auto l = spdlog::stdout_logger_mt("null");
  l->flush_on(spdlog::level::off);
  krbn::logger::set_logger(l);
}
} // namespace

TEST_CASE("manipulator.details.to_event_definition") {
  {
    nlohmann::json json;
    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::none);
    REQUIRE(event_definition.get_modifiers().size() == 0);
    REQUIRE(event_definition.get_lazy() == false);
    REQUIRE(event_definition.get_repeat() == true);
    REQUIRE(!(event_definition.get_event_definition().to_event()));
    REQUIRE(event_definition.make_modifier_events() == std::vector<krbn::event_queue::event>());
  }
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

  // key_code error

  {
    nlohmann::json json({
        {"key_code", nlohmann::json::array()},
        {"modifiers", {
                          "left_shift",
                      }},
    });
    REQUIRE_THROWS_AS(
        krbn::manipulator::to_event_definition(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::manipulator::to_event_definition(json),
        "`key_code` must be string, but is `[]`");
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
    nlohmann::json json;
    json["lazy"] = true;

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_lazy() == true);
  }
  // lazy
  {
    nlohmann::json json;
    json["repeat"] = false;

    krbn::manipulator::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_repeat() == false);
  }
}

TEST_CASE("event_definition.error_messages") {
  set_file_logger("tmp/error_messages.log");

  {
    std::ifstream json_file("json/error_messages.json");
    auto json = nlohmann::json::parse(json_file);
    std::vector<std::string> error_messages;
    for (const auto& j : json["to_event_definition"]) {
      try {
        krbn::manipulator::to_event_definition to_event_definition(j);
      } catch (const pqrs::json::unmarshal_error& e) {
        error_messages.push_back(e.what());
      } catch (const std::exception& e) {
        REQUIRE(false);
      }
    }

    REQUIRE(error_messages == std::vector<std::string>{
                                  "`modifiers` must be array or string, but is `{\"mandatory\":[\"shift\"]}`",
                                  "unknown modifier: `unknown`",
                                  "`shell_command` must be string, but is `1234`",
                                  "`select_input_source` must be object or array of objects, but is `1234`",
                                  "`set_variable` must be object, but is `1234`",
                                  "`set_variable.name` is not found in `{\"value\":1234}`",
                                  "`set_variable.name` must be string, but is `1234`",
                                  "`set_variable.value` is not found in `{\"name\":\"variable_name\"}`",
                                  "`set_variable.value` must be number, but is `\"1234\"`",
                              });
  }

  set_null_logger();
}
