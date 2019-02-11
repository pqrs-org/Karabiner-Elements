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

  if (!j.is_object()) {
    throw pqrs::json::unmarshal_error("json must be object, but is `"s + j.dump() + "`"s);
  }

  for (const auto& [key, value] : j.items()) {
    if (key == "input_source_id") {
      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error("`"s + key + "` must be string, but is `"s + value.dump() + "`"s);
      }
      p.set_input_source_id(value.get<std::string>());

    } else if (key == "localized_name") {
      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error("`"s + key + "` must be string, but is `"s + value.dump() + "`"s);
      }
      p.set_localized_name(value.get<std::string>());

    } else if (key == "input_mode_id") {
      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error("`"s + key + "` must be string, but is `"s + value.dump() + "`"s);
      }
      p.set_input_mode_id(value.get<std::string>());

    } else if (key == "languages") {
      if (!value.is_array()) {
        throw pqrs::json::unmarshal_error("`"s + key + "` must be array, but is `"s + value.dump() + "`"s);
      }

      std::vector<std::string> languages;
      for (const auto& v : value) {
        if (!v.is_string()) {
          throw pqrs::json::unmarshal_error("`"s + key + "` must be array of strings, but is `"s + value.dump() + "`"s);
        }
        languages.push_back(v.get<std::string>());
      }
      p.set_languages(languages);

    } else if (key == "first_language") {
      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error("`"s + key + "` must be string, but is `"s + value.dump() + "`"s);
      }
      p.set_first_language(value.get<std::string>());

    } else {
      throw pqrs::json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
