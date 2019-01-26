#pragma once

#include <nlohmann/json.hpp>
#include <pqrs/osx/input_source_selector/specifier.hpp>

namespace pqrs {
namespace osx {
namespace input_source_selector {
void to_json(nlohmann::json& j, const specifier& s) {
  j = nlohmann::json::object();

  if (auto& v = s.get_language_string()) {
    j["language"] = *v;
  }

  if (auto& v = s.get_input_source_id_string()) {
    j["input_source_id"] = *v;
  }

  if (auto& v = s.get_input_mode_id_string()) {
    j["input_mode_id"] = *v;
  }
}

void from_json(const nlohmann::json& j, specifier& s) {
  try {
    s.set_language(j.at("language").get<std::string>());
  } catch (...) {}

  try {
    s.set_input_source_id(j.at("input_source_id").get<std::string>());
  } catch (...) {}

  try {
    s.set_input_mode_id(j.at("input_mode_id").get<std::string>());
  } catch (...) {}
}
} // namespace input_source_selector
} // namespace osx
} // namespace pqrs
