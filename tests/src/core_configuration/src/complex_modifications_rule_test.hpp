#include "core_configuration/core_configuration.hpp"
#include <algorithm>
#include <boost/ut.hpp>

void run_complex_modifications_rule_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_literals;
  using namespace std::literals::string_view_literals;

  using rule_t = krbn::core_configuration::details::complex_modifications_rule;
  using parameters_t = krbn::core_configuration::details::complex_modifications_parameters;

  "complex_modifications_rule"_test = [] {
    // `parameters` in `manipulators` are not omitted in to_json.
    {
      auto json = R"(

{
  "description": "example",
  "description_notes": ["example note 1", "example note 2"],
  "manipulators": [
    {
      "from": { "key_code": "f12" },
      "to": [{ "key_code": "mission_control" }],
      "type": "basic",
      "parameters": {
        "basic.simultaneous_threshold_milliseconds": 50
      }
    }
  ]
}

)"_json;
      auto parameters = std::make_shared<parameters_t>();

      {
        rule_t rule(json,
                    parameters,
                    krbn::core_configuration::error_handling::strict);
        expect(1 == rule.get_manipulators().size());
        expect(rule.get_enabled());
        expect("example"s == rule.get_description());
        expect(std::vector<std::string>{"example note 1", "example note 2"} == rule.get_description_notes());
        expect(rule_t::code_type::json == rule.get_code_type());
        expect(krbn::json_utility::dump(json) == rule.get_code_string());
        expect(rule.get_code_string().contains("\"description_notes\": [\n"));
        expect(json == rule.to_json());
      }

      // with code_type
      {
        auto rule = rule_t::make_from_code(json.dump(),
                                           rule_t::code_type::json,
                                           parameters,
                                           krbn::core_configuration::error_handling::strict);
        expect(1 == rule->get_manipulators().size());
        expect(rule->get_enabled());
        expect("example"s == rule->get_description());
        expect(std::vector<std::string>{"example note 1", "example note 2"} == rule->get_description_notes());
        expect(rule_t::code_type::json == rule->get_code_type());
        expect(krbn::json_utility::dump(json) == rule->get_code_string());
        expect(json == rule->to_json());
      }
    }

    // enabled
    {
      auto json = R"(

{
  "description": "enabled",
  "enabled": false,
  "manipulators": [
    {
      "from": { "key_code": "f12" },
      "to": [{ "key_code": "mission_control" }],
      "type": "basic"
    }
  ]
}

)"_json;

      auto parameters = std::make_shared<parameters_t>();
      rule_t rule(json,
                  parameters,
                  krbn::core_configuration::error_handling::strict);
      expect(1 == rule.get_manipulators().size());
      expect(!rule.get_enabled());
      expect("enabled"s == rule.get_description());
      expect(json == rule.to_json());

      rule.set_enabled(true);
      expect(rule.get_enabled());

      auto expected_json = R"(

{
  "description": "enabled",
  "manipulators": [
    {
      "from": { "key_code": "f12" },
      "to": [{ "key_code": "mission_control" }],
      "type": "basic"
    }
  ]
}

)"_json;

      expect(expected_json == rule.to_json());
    }

    // search_text
    {
      auto manipulator = R"(

{
  "type": "basic",
  "to": [
    {
      "software_function": {
        "open_application": {
          "bundle_identifier": "com.example.Target"
        }
      }
    }
  ]
}

)"_json;
      auto json = nlohmann::json::object({
          {"description", "Search description"},
          {"enabled", false},
          {"available_since", nullptr},
          {"manipulators", nlohmann::json::array({manipulator})},
      });

      auto parameters = std::make_shared<parameters_t>();
      rule_t rule(json,
                  parameters,
                  krbn::core_configuration::error_handling::strict);

      auto expected = "Search description\n"s + manipulator.dump();
      expect(expected == rule.get_search_text());
    }

    // comparison_key
    {
      auto manipulator = R"(

{
  "type": "basic",
  "from": { "key_code": "f1" },
  "to": [{ "key_code": "f2" }]
}

)"_json;
      auto make_json_rule = [&](const std::string& description, bool enabled) {
        return rule_t(
            nlohmann::json::object({
                {"description", description},
                {"enabled", enabled},
                {"manipulators", nlohmann::json::array({manipulator})},
            }),
            std::make_shared<parameters_t>(),
            krbn::core_configuration::error_handling::strict);
      };

      auto rule1 = make_json_rule("first description", true);
      auto rule2 = make_json_rule("second description", false);
      expect(rule1.get_comparison_key() == rule2.get_comparison_key());

      auto throwing_javascript = "throw new Error('must not execute');"s;
      rule_t disabled_javascript(
          nlohmann::json::object({
              {"enabled", false},
              {"eval_js", throwing_javascript},
          }),
          std::make_shared<parameters_t>(),
          krbn::core_configuration::error_handling::strict);
      expect(disabled_javascript.get_comparison_key() ==
             nlohmann::json({
                                {"code_type", "javascript"},
                                {"source", throwing_javascript},
                            })
                 .dump());
    }

    //
    // eval_js
    //

    // normal
    {
      auto js = R"(

function main() {
  const rule = {
    description: "example",
    manipulators: [],
  };

  ["f10", "f11", "f12"].forEach(function (key_code) {
    rule.manipulators.push({
      type: "basic",
      from: {
        key_code: key_code,
        modifiers: {
          mandatory: ["right_shift"],
          optional: ["caps_lock"],
        },
      },
      to: [
        {
          key_code: key_code,
          modifiers: ["fn"],
        },
      ],
    });
  });

  return rule;
}

main();

)"s;

      {
        auto json = nlohmann::json::object({
            {"eval_js", js},
        });

        auto parameters = std::make_shared<parameters_t>();
        rule_t rule(json,
                    parameters,
                    krbn::core_configuration::error_handling::strict);
        expect(3 == rule.get_manipulators().size());
        expect("example"s == rule.get_description());
        expect(true == rule.get_enabled());
        expect(rule_t::code_type::javascript == rule.get_code_type());
        expect(js == rule.get_code_string());
        expect(json == rule.to_json());
      }

      // with code_type
      {
        auto parameters = std::make_shared<parameters_t>();
        auto rule = rule_t::make_from_code(js,
                                           rule_t::code_type::javascript,
                                           parameters,
                                           krbn::core_configuration::error_handling::strict);
        expect(3 == rule->get_manipulators().size());
        expect("example"s == rule->get_description());
        expect(true == rule->get_enabled());
        expect(rule_t::code_type::javascript == rule->get_code_type());
        expect(js == rule->get_code_string());
        expect(nlohmann::json::object({
                   {"eval_js", js},
               }) == rule->to_json());
      }

      // enabled
      {
        auto json = nlohmann::json::object({
            {"eval_js", js},
            {"enabled", false},
        });

        auto parameters = std::make_shared<parameters_t>();
        rule_t rule(json,
                    parameters,
                    krbn::core_configuration::error_handling::strict);
        expect(rule.get_manipulators().empty());
        expect("JavaScript"s == rule.get_description());
        expect(false == rule.get_enabled());
        expect(rule_t::code_type::javascript == rule.get_code_type());
        expect(js == rule.get_code_string());
        expect(json == rule.to_json());
        rule.set_enabled(true);
        expect(rule.get_enabled());
        expect(3 == rule.get_manipulators().size());
        expect("example"s == rule.get_description());
      }

      // Ignore enabled in the evaluated JSON.
      {
        auto js_with_enabled = R"(

function main() {
  return {
    description: "example",
    enabled: false,
    manipulators: [
      {
        type: "basic",
        from: { key_code: "spacebar" },
        to: [{ key_code: "tab" }],
      },
    ],
  };
}

main();

)"s;

        auto parameters = std::make_shared<parameters_t>();
        auto rule = rule_t::make_from_code(js_with_enabled,
                                           rule_t::code_type::javascript,
                                           parameters,
                                           krbn::core_configuration::error_handling::strict);
        expect(rule->get_enabled());
        expect(nlohmann::json::object({
                   {"eval_js", js_with_enabled},
               }) == rule->to_json());
      }
    }

    // Disabled JavaScript is not evaluated, even when it throws. Enabling it
    // reports the error without changing the stored rule or enabled state.
    {
      auto json = nlohmann::json::object({
          {"enabled", false},
          {"description", "Disabled script"},
          {"eval_js", "throw new Error('must not execute');"},
      });
      rule_t rule(json, std::make_shared<parameters_t>(), krbn::core_configuration::error_handling::strict);
      expect(!rule.get_enabled());
      expect(rule.get_manipulators().empty());
      expect(rule.get_description() == "Disabled script");
      expect(rule.to_json() == json);
      expect(throws([&] { rule.set_enabled(true); }));
      expect(!rule.get_enabled());
      expect(rule.to_json() == json);
    }

    // search_text uses the evaluated manipulators, not the JavaScript source.
    {
      auto js = R"(

function main() {
  const source_only_marker = "not included in the result";

  return {
    description: "JavaScript search description",
    manipulators: [
      {
        type: "basic",
        from: { key_code: "f1" },
        to: [{ key_code: "display_brightness_decrement" }],
      },
    ],
  };
}

main();

)"s;
      auto manipulator = R"(

{
  "type": "basic",
  "from": { "key_code": "f1" },
  "to": [{ "key_code": "display_brightness_decrement" }]
}

)"_json;

      auto parameters = std::make_shared<parameters_t>();
      auto rule = rule_t::make_from_code(js,
                                         rule_t::code_type::javascript,
                                         parameters,
                                         krbn::core_configuration::error_handling::strict);

      auto expected = "JavaScript search description\n"s + manipulator.dump();
      expect(expected == rule->get_search_text());
      expect(rule->get_search_text().find("source_only_marker") == std::string::npos);
    }

    // error
    {
      auto js = R"(

{

)"s;

      auto json = nlohmann::json::object({
          {"eval_js", js},
      });

      auto parameters = std::make_shared<parameters_t>();
      try {
        rule_t rule(json,
                    parameters,
                    krbn::core_configuration::error_handling::strict);
        expect(false);
      } catch (pqrs::json::unmarshal_error& ex) {
        expect("`eval_js` error: javascript error: SyntaxError: parse error (line 5, end of input)"sv == ex.what());
      } catch (...) {
        expect(false);
      }
    }
  };
}
