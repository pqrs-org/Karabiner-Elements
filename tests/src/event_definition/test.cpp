#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "filesystem.hpp"
#include "manipulator/details/basic.hpp"
#include "manipulator/details/types.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_command_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_command,
                                                                 krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_control_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_control,
                                                                 krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_option_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                krbn::modifier_flag::left_option,
                                                                krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                               krbn::modifier_flag::left_shift,
                                                               krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag right_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                krbn::modifier_flag::right_shift,
                                                                krbn::device_id(1));

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

TEST_CASE("modifier_definition.make_modifiers") {
  using krbn::manipulator::details::event_definition;
  using krbn::manipulator::details::modifier_definition;

  {
    nlohmann::json json({"left_command", "left_shift", "fn", "any"});
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
        modifier_definition::modifier::left_shift,
        modifier_definition::modifier::fn,
        modifier_definition::modifier::any,
    });
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json("left_command");
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
    });
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json("unknown");
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({});
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json(nullptr);
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({});
    REQUIRE(actual == expected);
  }
}

TEST_CASE("modifier_definition.get_modifier") {
  using krbn::manipulator::details::modifier_definition;

  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::zero) == modifier_definition::modifier::end_);
  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::left_shift) == modifier_definition::modifier::left_shift);
}

TEST_CASE("manipulator.details.to_event_definition") {
  {
    nlohmann::json json;
    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::none);
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
    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definition().get_key_code() == krbn::key_code::spacebar);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::modifier_definition::modifier>({
                                                    krbn::manipulator::details::modifier_definition::modifier::shift,
                                                    krbn::manipulator::details::modifier_definition::modifier::left_command,
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
    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definition().get_key_code() == krbn::key_code::right_option);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_modifiers() == std::unordered_set<krbn::manipulator::details::modifier_definition::modifier>({
                                                    krbn::manipulator::details::modifier_definition::modifier::shift,
                                                    krbn::manipulator::details::modifier_definition::modifier::left_command,
                                                }));
    REQUIRE(event_definition.make_modifier_events() == std::vector<krbn::event_queue::event>({
                                                           krbn::event_queue::event(krbn::key_code::left_command),
                                                           krbn::event_queue::event(krbn::key_code::left_shift),
                                                       }));
  }
  {
    nlohmann::json json({
        {"key_code", nlohmann::json::array()},
        {"modifiers", {
                          "dummy",
                      }},
    });
    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::none);
    REQUIRE(event_definition.get_event_definition().get_key_code() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_modifiers().size() == 0);
    REQUIRE(event_definition.make_modifier_events() == std::vector<krbn::event_queue::event>({}));
  }
  {
    std::string shell_command = "open /Applications/Safari.app";
    nlohmann::json json({
        {"shell_command", shell_command},
    });
    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::shell_command);
    REQUIRE(event_definition.get_event_definition().get_key_code() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == shell_command);
    REQUIRE(event_definition.get_event_definition().get_input_source_selectors() == boost::none);
  }
  // select_input_source
  {
    krbn::input_source_selector input_source_selector(boost::none,
                                                      std::string("com.apple.keylayout.US"),
                                                      boost::none);

    nlohmann::json json;
    json["select_input_source"]["input_source_id"] = "com.apple.keylayout.US";

    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::select_input_source);
    REQUIRE(event_definition.get_event_definition().get_key_code() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_input_source_selectors() == std::vector<krbn::input_source_selector>({input_source_selector}));
  }
  // select_input_source (array)
  {
    krbn::input_source_selector input_source_selector1(boost::none,
                                                       std::string("com.apple.keylayout.US"),
                                                       boost::none);
    krbn::input_source_selector input_source_selector2(std::string("en"),
                                                       boost::none,
                                                       boost::none);

    nlohmann::json json;
    json["select_input_source"] = nlohmann::json::array();
    json["select_input_source"].push_back(nlohmann::json::object());
    json["select_input_source"].back()["input_source_id"] = "com.apple.keylayout.US";
    json["select_input_source"].push_back(nlohmann::json::object());
    json["select_input_source"].back()["language"] = "en";

    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_event_definition().get_type() == krbn::manipulator::details::event_definition::type::select_input_source);
    REQUIRE(event_definition.get_event_definition().get_key_code() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_pointing_button() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_shell_command() == boost::none);
    REQUIRE(event_definition.get_event_definition().get_input_source_selectors() == std::vector<krbn::input_source_selector>({input_source_selector1,
                                                                                                                              input_source_selector2}));
  }
  // lazy
  {
    nlohmann::json json;
    json["lazy"] = true;

    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_lazy() == true);
  }
  // lazy
  {
    nlohmann::json json;
    json["repeat"] = false;

    krbn::manipulator::details::to_event_definition event_definition(json);
    REQUIRE(event_definition.get_repeat() == false);
  }
}

TEST_CASE("event_definition.error_messages") {
  set_file_logger("tmp/error_messages.log");

  {
    std::ifstream json_file("json/error_messages.json");
    auto json = nlohmann::json::parse(json_file);
    for (const auto& j : json["to_event_definition"]) {
      krbn::manipulator::details::to_event_definition to_event_definition(j);
    }
  }

  set_null_logger();
}
