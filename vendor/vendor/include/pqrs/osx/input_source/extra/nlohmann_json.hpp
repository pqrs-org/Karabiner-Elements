#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/json.hpp>
#include <pqrs/osx/input_source/properties.hpp>

namespace pqrs {
namespace osx {
namespace input_source {
inline void to_json(nlohmann::json& j, const properties& p) {
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

inline void from_json(const nlohmann::json& j, properties& p) {
  using namespace std::string_literals;

  json::requires_object(j, "json");

  for (const auto& [key, value] : j.items()) {
    if (key == "input_source_id") {
      json::requires_string(value, "`"s + key + "`");

      p.set_input_source_id(value.get<std::string>());

    } else if (key == "localized_name") {
      json::requires_string(value, "`"s + key + "`");

      p.set_localized_name(value.get<std::string>());

    } else if (key == "input_mode_id") {
      json::requires_string(value, "`"s + key + "`");

      p.set_input_mode_id(value.get<std::string>());

    } else if (key == "languages") {
      json::requires_array(value, "`"s + key + "`");

      std::vector<std::string> languages;
      for (const auto& v : value) {
        json::requires_string(v, "`"s + key + "` entry");

        languages.push_back(v.get<std::string>());
      }
      p.set_languages(languages);

    } else if (key == "first_language") {
      json::requires_string(value, "`"s + key + "`");

      p.set_first_language(value.get<std::string>());

    } else {
      throw json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
