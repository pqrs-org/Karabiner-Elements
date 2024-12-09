#pragma once

#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
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
}; // namespace complex_modifications_utility
} // namespace krbn
