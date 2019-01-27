#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
#include <pqrs/osx/input_source/properties.hpp>

namespace pqrs {
namespace osx {
namespace input_source {
void to_json(nlohmann::json& j, const properties& p) {
  j = nlohmann::json::object();

  if (auto& v = p.get_input_source_id()) {
    j["input_source_id"] = *v;
  }

  if (auto& v = p.get_localized_name()) {
    j["localized_name"] = *v;
  }

  if (auto& v = p.get_input_mode_id()) {
    j["input_mode_id"] = *v;
  }

  if (!p.get_languages().empty()) {
    j["languages"] = p.get_languages();
  }

  if (auto& v = p.get_first_language()) {
    j["first_language"] = *v;
  }
}

void from_json(const nlohmann::json& j, properties& p) {
  try {
    p.set_input_source_id(j.at("input_source_id").get<std::string>());
  } catch (...) {}

  try {
    p.set_localized_name(j.at("localized_name").get<std::string>());
  } catch (...) {}

  try {
    p.set_input_mode_id(j.at("input_mode_id").get<std::string>());
  } catch (...) {}

  try {
    p.set_languages(j.at("languages").get<std::vector<std::string>>());
  } catch (...) {}

  try {
    p.set_first_language(j.at("first_language").get<std::string>());
  } catch (...) {}
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
