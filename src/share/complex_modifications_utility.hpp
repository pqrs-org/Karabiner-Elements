#pragma once

#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulator_factory.hpp"

namespace krbn {
namespace complex_modifications_utility {
inline std::vector<std::string> lint_rule(const core_configuration::details::complex_modifications_rule& rule) {
  std::vector<std::string> error_messages;

  for (const auto& manipulator : rule.get_manipulators()) {
    try {
      manipulator::manipulator_factory::make_manipulator(manipulator.get_json(),
                                                         manipulator.get_parameters());
      for (const auto& c : manipulator.get_conditions()) {
        manipulator::manipulator_factory::make_condition(c.get_json());
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
  nlohmann::json json({{"description", "New Rule"},
                       {"manipulators", nlohmann::json::array({nlohmann::json::object({
                                            {
                                                "from",
                                                nlohmann::json::object({
                                                    {"key_code", "REPLACE_ME"},
                                                    {"modifiers", nlohmann::json::object({
                                                                      {"mandatory", nlohmann::json::array({})},
                                                                      {"optional", nlohmann::json::array({"any"})},
                                                                  })},
                                                }),
                                            },
                                            {
                                                "to",
                                                nlohmann::json::array({
                                                    nlohmann::json::object({
                                                        {"key_code", "REPLACE_ME"},
                                                        {"modifiers", nlohmann::json::array({})},
                                                    }),

                                                }),
                                            },
                                            {"type", "basic"},
                                        })})}});

  return json_utility::dump(json);
}
}; // namespace complex_modifications_utility
} // namespace krbn
