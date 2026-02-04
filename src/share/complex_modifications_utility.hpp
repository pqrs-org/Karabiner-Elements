#pragma once

#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "json_writer.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/manipulator_factory.hpp"

namespace krbn {
namespace complex_modifications_utility {
inline std::vector<std::string> lint_rule(const core_configuration::details::complex_modifications_rule& rule) {
  std::vector<std::string> error_messages;

  for (const auto& m : rule.get_manipulators()) {
    try {
      manipulator::manipulator_factory::make_manipulator(m->to_json(),
                                                         m->get_parameters());
      for (const auto& c : m->get_conditions()) {
        manipulator::condition_factory::make_condition(c.get_json());
      }

    } catch (const std::exception& e) {
      error_messages.push_back(fmt::format("`{0}` error: {1}",
                                           rule.get_description(),
                                           e.what()));
    }
  }

  return error_messages;
}

inline std::string get_new_rule_json_string(void) {
  nlohmann::json json({{"description", "New Rule (change left_shift+caps_lock to page_down, right_shift+caps_lock to left_command+mission_control)"},
                       {"manipulators", nlohmann::json::array({
                                            nlohmann::json::object({
                                                {
                                                    "from",
                                                    nlohmann::json::object({
                                                        {"key_code", "caps_lock"},
                                                        {"modifiers", nlohmann::json::object({
                                                                          {"mandatory", nlohmann::json::array({"left_shift"})},
                                                                          {"optional", nlohmann::json::array({"any"})},
                                                                      })},
                                                    }),
                                                },
                                                {
                                                    "to",
                                                    nlohmann::json::array({
                                                        nlohmann::json::object({
                                                            {"key_code", "page_down"},
                                                            {"modifiers", nlohmann::json::array({})},
                                                        }),

                                                    }),
                                                },
                                                {"type", "basic"},
                                            }),
                                            nlohmann::json::object({
                                                {
                                                    "from",
                                                    nlohmann::json::object({
                                                        {"key_code", "caps_lock"},
                                                        {"modifiers", nlohmann::json::object({
                                                                          {"mandatory", nlohmann::json::array({"right_shift"})},
                                                                          {"optional", nlohmann::json::array({"any"})},
                                                                      })},
                                                    }),
                                                },
                                                {
                                                    "to",
                                                    nlohmann::json::array({
                                                        nlohmann::json::object({
                                                            {"apple_vendor_keyboard_key_code", "mission_control"},
                                                            {"modifiers", nlohmann::json::array({"left_command"})},
                                                        }),
                                                    }),
                                                },
                                                {"type", "basic"},
                                            }),
                                        })}});

  return json_utility::dump(json);
}

inline std::string get_new_rule_eval_js_string(void) {
  return R"(// JavaScript must be written in ECMAScript 5.1.

function main() {
  const apps = [
    { key_code: '1', app: 'com.apple.Safari' },
    { key_code: '2', app: 'com.apple.TextEdit' },
    { key_code: '3', app: 'com.apple.ActivityMonitor' },
  ]

  const manipulators = []
  for (var i = 0; i < apps.length; ++i) {
    const app = apps[i]

    console.log(app.key_code + ' to ' + app.app)

    manipulators.push({
      type: 'basic',
      from: {
        key_code: app.key_code,
        modifiers: {
          mandatory: ['right_shift'],
        },
      },
      to: [
        {
          software_function: {
            open_application: { bundle_identifier: app.app },
          },
        },
      ],
    })
  }

  return {
    description: 'Open apps with right_shift+1/2/3',
    manipulators: manipulators,
  }
}

main()
)";
}

// Save .prettierrc.json to ~/.local/share/karabiner for external editors.
void save_prettierrc(void) {
  auto directory = constants::get_user_data_directory();
  if (!directory.empty()) {
    auto json = nlohmann::ordered_json::object({
        {"semi", false},
        {"singleQuote", true},
        {"tabWidth", 2},
        {"printWidth", 200},
        {"trailingComma", "es5"},
        {
            "overrides",
            nlohmann::json::array(
                {nlohmann::ordered_json::object({
                    {"files", "*.js"},
                    {"options", nlohmann::ordered_json::object({
                                    {"tabWidth", 2},
                                })},
                })}),
        },
    });

    // Note: The actual json and js files are placed in ~/.local/share/karabiner/tmp/XXXX.json, but
    // to allow users to override with ~/.local/share/karabiner/tmp/.prettierrc.json,
    // place the default .prettierrc.json in the parent directory.
    json_writer::sync_save_to_file(json,
                                   directory / ".prettierrc.json",
                                   0700,
                                   0600);
  }
}
}; // namespace complex_modifications_utility
} // namespace krbn
