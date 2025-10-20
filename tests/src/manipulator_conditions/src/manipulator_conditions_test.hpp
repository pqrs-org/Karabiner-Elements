#pragma once

#include "../../share/manipulator_conditions_helper.hpp"
#include "actual_examples_helper.hpp"
#include <pqrs/gsl.hpp>

void run_manipulator_conditions_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "manipulator.condition_factory"_test = [] {
    {
      nlohmann::json json({
          {"type", "frontmost_application_if"},
          {
              "bundle_identifiers",
              {
                  "^com\\.apple\\.Terminal$",
                  "^com\\.googlecode\\.iterm2$",
              },
          },
      });
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::frontmost_application*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json({
          {"type", "input_source_if"},
      });
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::input_source*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json({
          {"type", "input_source_unless"},
      });
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::input_source*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json;
      json["type"] = "input_source_if";
      json["input_sources"] = nlohmann::json::array();
      {
        nlohmann::json j;
        j["language"] = "^en$";
        j["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";
        j["input_mode_id"] = "^com\\.apple\\.inputmethod\\.SCIM\\.ITABC$";
        json["input_sources"].push_back(j);
      }
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::input_source*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json({
          {"type", "variable_if"},
          {"name", "variable_name"},
          {"value", 1},
      });
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::variable*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json({
          {"type", "variable_unless"},
          {"name", "variable_name"},
          {"value", 1},
      });
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::variable*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json;
      json["type"] = "keyboard_type_if";
      json["keyboard_types"] = nlohmann::json::array({"iso"});
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::keyboard_type*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
    {
      nlohmann::json json;
      json["type"] = "keyboard_type_unless";
      json["keyboard_types"] = nlohmann::json::array({"iso"});
      auto condition = krbn::manipulator::condition_factory::make_condition(json);
      auto p = pqrs::unwrap_not_null(condition).get();
      expect(dynamic_cast<krbn::manipulator::conditions::keyboard_type*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::conditions::nop*>(p) == nullptr);
    }
  };

  "conditions.expression"_test = [] {
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry entry(krbn::device_id(1),
                                   krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_type::key_down,
                                   krbn::event_integer_value::value_t(1),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_queue::state::original);

    //
    // expression_if
    //

    {
      krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_if",
  "expression": "2 > 1"
}

)"_json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }

    // nan
    {
      try {
        krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_if",
  "expression": "cos("
}

)"_json);
        expect(false) << "json must throw unmarshal_error for invalid expression";
      } catch (const pqrs::json::unmarshal_error& e) {
        expect(R"(`expression` error: invalid expression `"cos("`)"sv == e.what());
      }
    }

    // no expression
    {
      krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_if"
}

)"_json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }

    //
    // expression_unless
    //

    {
      krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_unless",
  "expression": "2 < 1"
}

)"_json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }

    // nan
    {
      try {
        krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_unless",
  "expression": "cos("
}

)"_json);
        expect(false) << "json must throw unmarshal_error for invalid expression";
      } catch (const pqrs::json::unmarshal_error& e) {
        expect(R"(`expression` error: invalid expression `"cos("`)"sv == e.what());
      }
    }

    // no expression
    {
      krbn::manipulator::conditions::expression condition(R"(
{
  "type": "expression_unless"
}

)"_json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
  };

  "conditions.frontmost_application"_test = [] {
    actual_examples_helper helper("frontmost_application.json");
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry entry(krbn::device_id(1),
                                   krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_type::key_down,
                                   krbn::event_integer_value::value_t(1),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_queue::state::original);

    // bundle_identifiers matching
    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com.apple.Terminal");
      application.set_file_path("/not_found");
      manipulator_environment.set_frontmost_application(application);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
      // use cache
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    // Test regex escape works properly
    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com/apple/Terminal");
      application.set_file_path("/not_found");
      manipulator_environment.set_frontmost_application(application);

      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
      // use cache
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
    }

    // file_path matching
    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com/apple/Terminal");
      application.set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
      manipulator_environment.set_frontmost_application(application);

      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    // frontmost_application_unless
    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com.googlecode.iterm2");
      application.set_file_path("/Applications/iTerm.app");
      manipulator_environment.set_frontmost_application(application);

      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com.googlecode.iterm2");
      application.set_file_path("/Users/tekezo/Applications/iTerm.app");
      manipulator_environment.set_frontmost_application(application);

      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
    }
  };

  "conditions.input_source"_test = [] {
    actual_examples_helper helper("input_source.json");
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry entry(krbn::device_id(1),
                                   krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_type::key_down,
                                   krbn::event_integer_value::value_t(1),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_queue::state::original);

    // language matching
    {
      pqrs::osx::input_source::properties properties;
      properties.set_first_language("en");
      properties.set_input_source_id("com.apple.keylayout.Australian");
      manipulator_environment.set_input_source_properties(properties);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
      // use cache
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    // Test regex escape works properly
    {
      pqrs::osx::input_source::properties properties;
      properties.set_first_language("ja");
      properties.set_input_source_id("com/apple/keylayout/Australian");
      manipulator_environment.set_input_source_properties(properties);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
      // use cache
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
    }

    // input_source_id matching
    {
      pqrs::osx::input_source::properties properties;
      properties.set_first_language("ja");
      properties.set_input_source_id("com.apple.keylayout.US");
      manipulator_environment.set_input_source_properties(properties);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    // input_mode_id matching
    {
      pqrs::osx::input_source::properties properties;
      properties.set_first_language("ja");
      properties.set_input_source_id("com.apple.keylayout.Australian");
      properties.set_input_mode_id("com.apple.inputmethod.Japanese.FullWidthRoman");
      manipulator_environment.set_input_source_properties(properties);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }

    // input_source_unless
    {
      pqrs::osx::input_source::properties properties;
      properties.set_first_language("fr");
      properties.set_input_source_id("com.apple.keylayout.US");
      manipulator_environment.set_input_source_properties(properties);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
    }
  };

  "conditions.event_changed"_test = [] {
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry original_entry(krbn::device_id(1),
                                            krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                            krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                  pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                            krbn::event_type::key_down,
                                            krbn::event_integer_value::value_t(1),
                                            krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                  pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                            krbn::event_queue::state::original);
    krbn::event_queue::entry manipulated_entry(krbn::device_id(1),
                                               krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                               krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                               krbn::event_type::key_down,
                                               krbn::event_integer_value::value_t(1),
                                               krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                               krbn::event_queue::state::manipulated);

    {
      actual_examples_helper helper("event_changed_if.json");

      expect(helper.get_condition_manager().is_fulfilled(original_entry,
                                                         manipulator_environment) == true);

      expect(helper.get_condition_manager().is_fulfilled(manipulated_entry,
                                                         manipulator_environment) == false);
    }
    {
      actual_examples_helper helper("event_changed_unless.json");

      expect(helper.get_condition_manager().is_fulfilled(original_entry,
                                                         manipulator_environment) == true);

      expect(helper.get_condition_manager().is_fulfilled(manipulated_entry,
                                                         manipulator_environment) == false);
    }
  };

  "conditions.keyboard_type"_test = [] {
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry entry(krbn::device_id(1),
                                   krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_type::key_down,
                                   krbn::event_integer_value::value_t(1),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_queue::state::original);

    auto core_configuration = std::make_shared<krbn::core_configuration::core_configuration>();
    manipulator_environment.set_core_configuration(core_configuration);

    {
      actual_examples_helper helper("keyboard_type_if.json");

      // iso
      core_configuration->get_selected_profile().get_virtual_hid_keyboard()->set_keyboard_type_v2("iso");
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);

      // ansi
      core_configuration->get_selected_profile().get_virtual_hid_keyboard()->set_keyboard_type_v2("ansi");
      manipulator_environment.set_core_configuration(core_configuration);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);
    }
    {
      actual_examples_helper helper("keyboard_type_unless.json");

      // iso
      core_configuration->get_selected_profile().get_virtual_hid_keyboard()->set_keyboard_type_v2("iso");
      manipulator_environment.set_core_configuration(core_configuration);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == false);

      // ansi
      core_configuration->get_selected_profile().get_virtual_hid_keyboard()->set_keyboard_type_v2("ansi");
      manipulator_environment.set_core_configuration(core_configuration);
      expect(helper.get_condition_manager().is_fulfilled(entry,
                                                         manipulator_environment) == true);
    }
  };

  "conditions.variable"_test = [] {
    krbn::manipulator::manipulator_environment manipulator_environment;
    krbn::event_queue::entry entry(krbn::device_id(1),
                                   krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_type::key_down,
                                   krbn::event_integer_value::value_t(1),
                                   krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                   krbn::event_queue::state::original);

    manipulator_environment.set_variable("variable_name", krbn::manipulator_environment_variable_value(123));
    manipulator_environment.set_variable("variable_bool", krbn::manipulator_environment_variable_value(true));
    manipulator_environment.set_variable("variable_string", krbn::manipulator_environment_variable_value("hello"));

    //
    // variable_if
    //

    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_name";
      json["value"] = 123;
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_name";
      json["value"] = 1234;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "unknown";
      json["value"] = 0;
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "unknown";
      json["value"] = 1;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_bool";
      json["value"] = 1234;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_bool";
      json["value"] = true;
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_string";
      json["value"] = true;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_if";
      json["name"] = "variable_string";
      json["value"] = "hello";
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }

    //
    // variable_unless
    //

    {
      nlohmann::json json;
      json["type"] = "variable_unless";
      json["name"] = "variable_name";
      json["value"] = 123;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_unless";
      json["name"] = "variable_name";
      json["value"] = 1234;
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_unless";
      json["name"] = "unknown";
      json["value"] = 0;
      krbn::manipulator::conditions::variable condition(json);

      expect(!condition.is_fulfilled(entry, manipulator_environment));
    }
    {
      nlohmann::json json;
      json["type"] = "variable_unless";
      json["name"] = "unknown";
      json["value"] = 1;
      krbn::manipulator::conditions::variable condition(json);

      expect(condition.is_fulfilled(entry, manipulator_environment));
    }
  };
}
