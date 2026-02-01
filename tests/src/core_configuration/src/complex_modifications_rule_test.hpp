#include "core_configuration/core_configuration.hpp"
#include <algorithm>
#include <boost/ut.hpp>

void run_complex_modifications_rule_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_literals;
  using namespace std::literals::string_view_literals;

  "complex_modifications_rule"_test = [] {
    // `parameters` in `manipulators` are not omitted in to_json.
    {
      auto json = R"(

{
  "description": "example",
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
      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      krbn::core_configuration::details::complex_modifications_rule rule(json,
                                                                         parameters,
                                                                         krbn::core_configuration::error_handling::strict);
      expect(1 == rule.get_manipulators().size());
      expect(rule.get_enabled());
      expect("example"s == rule.get_description());
      expect(json == rule.to_json());
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

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      krbn::core_configuration::details::complex_modifications_rule rule(json,
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

      auto json = nlohmann::json::object({
          {"eval_js", js},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      krbn::core_configuration::details::complex_modifications_rule rule(json,
                                                                         parameters,
                                                                         krbn::core_configuration::error_handling::strict);
      expect(3 == rule.get_manipulators().size());
      expect("example"s == rule.get_description());
      expect(json == rule.to_json());
    }

    // error
    {
      auto js = R"(

{

)"s;

      auto json = nlohmann::json::object({
          {"eval_js", js},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      try {
        krbn::core_configuration::details::complex_modifications_rule rule(json,
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
