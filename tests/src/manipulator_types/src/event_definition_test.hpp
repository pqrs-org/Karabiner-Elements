#include "manipulator/types.hpp"
#include <boost/ut.hpp>

void run_event_definition_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_view_literals;

  "to_event_definition"_test = [] {
    {
      nlohmann::json json({
          {"key_code", "spacebar"},
          {"modifiers", nlohmann::json::array({
                            "shift",
                            "left_command",
                        })},
      });
      krbn::manipulator::to_event_definition event_definition(json);
      expect(event_definition.get_event_definition().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      expect(event_definition.get_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                     krbn::manipulator::modifier_definition::modifier::shift,
                                                     krbn::manipulator::modifier_definition::modifier::left_command,
                                                 }));
      expect(event_definition.get_event_definition().to_event() ==
             krbn::event_queue::event(
                 krbn::momentary_switch_event(
                     pqrs::hid::usage_page::keyboard_or_keypad,
                     pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar)));
      expect(event_definition.make_modifier_events() ==
             std::vector<krbn::momentary_switch_event>({
                 krbn::momentary_switch_event(
                     pqrs::hid::usage_page::keyboard_or_keypad,
                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui),
                 krbn::momentary_switch_event(
                     pqrs::hid::usage_page::keyboard_or_keypad,
                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift),
             }));
      expect(event_definition.get_condition_manager().get_conditions().empty());
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
      expect(event_definition.get_event_definition().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt));
      expect(event_definition.get_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                     krbn::manipulator::modifier_definition::modifier::shift,
                                                     krbn::manipulator::modifier_definition::modifier::left_command,
                                                 }));
      expect(event_definition.make_modifier_events() ==
             std::vector<krbn::momentary_switch_event>({
                 krbn::momentary_switch_event(
                     pqrs::hid::usage_page::keyboard_or_keypad,
                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui),
                 krbn::momentary_switch_event(
                     pqrs::hid::usage_page::keyboard_or_keypad,
                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift),
             }));
    }

    {
      std::string shell_command = "open /Applications/Safari.app";
      nlohmann::json json({
          {"shell_command", shell_command},
      });
      krbn::manipulator::to_event_definition event_definition(json);
      expect(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::shell_command);
      expect(event_definition.get_event_definition().get_if<krbn::momentary_switch_event>() == nullptr);
      expect(event_definition.get_event_definition().get_shell_command() == shell_command);
      expect(event_definition.get_event_definition().get_input_source_specifiers() == std::nullopt);
    }
    // select_input_source
    {
      pqrs::osx::input_source_selector::specifier s;
      s.set_input_source_id("^com\\.apple\\.keylayout\\.US$");

      nlohmann::json json;
      json["select_input_source"]["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";

      krbn::manipulator::to_event_definition event_definition(json);
      expect(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::select_input_source);
      expect(event_definition.get_event_definition().get_if<krbn::momentary_switch_event>() == nullptr);
      expect(event_definition.get_event_definition().get_shell_command() == std::nullopt);
      expect(event_definition.get_event_definition().get_input_source_specifiers() == std::vector<pqrs::osx::input_source_selector::specifier>({s}));
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
      expect(event_definition.get_event_definition().get_type() == krbn::manipulator::event_definition::type::select_input_source);
      expect(event_definition.get_event_definition().get_if<krbn::momentary_switch_event>() == nullptr);
      expect(event_definition.get_event_definition().get_shell_command() == std::nullopt);
      expect(event_definition.get_event_definition().get_input_source_specifiers() == specifiers);
    }
    // lazy
    {
      auto json = nlohmann::json::object({
          {"key_code", "left_shift"},
          {"lazy", true},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      expect(event_definition.get_lazy() == true);
    }
    // lazy
    {
      auto json = nlohmann::json::object({
          {"key_code", "a"},
          {"repeat", false},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      expect(event_definition.get_repeat() == false);
    }
    // sticky_modifier
    {
      auto json = nlohmann::json::object({
          {"sticky_modifier", nlohmann::json::object({{"left_shift", "toggle"}})},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_if<std::pair<krbn::modifier_flag, krbn::sticky_modifier_type>>();
      expect(pair->first == krbn::modifier_flag::left_shift);
      expect(pair->second == krbn::sticky_modifier_type::toggle);
    }
    // conditions
    {
      auto json = R"(

{
  "key_code": "6",
  "modifiers": ["left_shift"],
  "conditions": [
    {
      "keyboard_types": ["ansi", "iso"],
      "type": "keyboard_type_if"
    }
  ]
}

      )"_json;

      krbn::manipulator::to_event_definition event_definition(json);
      expect(1 == event_definition.get_condition_manager().get_conditions().size());
    }
    // set_variable
    {
      auto json = nlohmann::json::object({
          {"set_variable", nlohmann::json::object({
                               {"name", "variable1"},
                               {"value", 42},
                           })},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_set_variable();
      expect(pair != std::nullopt);
      expect("variable1"sv == pair->get_name());
      expect(krbn::manipulator_environment_variable_value(42) == pair->get_value());
    }
    {
      auto json = nlohmann::json::object({
          {"set_variable", nlohmann::json::object({
                               {"name", "variable2"},
                               {"value", true},
                           })},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_set_variable();
      expect(pair != std::nullopt);
      expect("variable2"sv == pair->get_name());
      expect(krbn::manipulator_environment_variable_value(true) == pair->get_value());
    }
    {
      auto json = nlohmann::json::object({
          {"set_variable", nlohmann::json::object({
                               {"name", "variable3"},
                               {"value", "on"},
                           })},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_set_variable();
      expect(pair != std::nullopt);
      expect("variable3"sv == pair->get_name());
      expect(krbn::manipulator_environment_variable_value("on") == pair->get_value());
    }
    // key_up_value only
    {
      auto json = nlohmann::json::object({
          {"set_variable", nlohmann::json::object({
                               {"name", "variable4"},
                               {"key_up_value", "up"},
                           })},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_set_variable();
      expect(pair != std::nullopt);
      expect("variable4"sv == pair->get_name());
      expect(krbn::manipulator_environment_variable_value("up") == pair->get_key_up_value());
    }
    // type::unset
    {
      auto json = nlohmann::json::object({
          {"set_variable", nlohmann::json::object({
                               {"name", "variable5"},
                               {"type", "unset"},
                           })},
      });

      krbn::manipulator::to_event_definition event_definition(json);
      auto pair = event_definition.get_event_definition().get_set_variable();
      expect(pair != std::nullopt);
      expect("variable5"sv == pair->get_name());
      expect(krbn::manipulator_environment_variable_set_variable::type::unset == pair->get_type());
    }
  };
}
