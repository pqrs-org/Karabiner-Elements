#pragma once

// pqrs::json v1.7

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "json/formatter.hpp"
#include "json/marshal_error.hpp"
#include "json/pqrs_formatter.hpp"
#include "json/unmarshal_error.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/string.hpp>

namespace pqrs {
namespace json {
template <typename T>
inline std::optional<T> find(const nlohmann::json& json,
                             const std::string& key) {
  auto it = json.find(key);
  if (it != std::end(json)) {
    try {
      return it.value().get<T>();
    } catch (std::exception&) {
    }
  }

  return std::nullopt;
}

inline std::optional<nlohmann::json::const_iterator> find_array(const nlohmann::json& json,
                                                                const std::string& key) {
  auto it = json.find(key);
  if (it != std::end(json)) {
    if (it->is_array()) {
      return it;
    }
  }
  return std::nullopt;
}

inline std::optional<nlohmann::json::const_iterator> find_object(const nlohmann::json& json,
                                                                 const std::string& key) {
  auto it = json.find(key);
  if (it != std::end(json)) {
    if (it->is_object()) {
      return it;
    }
  }
  return std::nullopt;
}

inline std::optional<nlohmann::json::const_iterator> find_json(const nlohmann::json& json,
                                                               const std::string& key) {
  auto it = json.find(key);
  if (it != std::end(json)) {
    return it;
  }
  return std::nullopt;
}

inline nlohmann::json find_copy(const nlohmann::json& json,
                                const std::string& key,
                                const nlohmann::json& fallback_value) {
  auto it = json.find(key);
  if (it != std::end(json)) {
    return it.value();
  }
  return fallback_value;
}

//
// type checks
//

inline std::string dump_for_error_message(const nlohmann::json& json) {
  return string::truncate(json.dump(), 256);
}

inline void requires_array(const nlohmann::json& json, const std::string& error_message_name) {
  using namespace std::string_literals;

  if (!json.is_array()) {
    throw unmarshal_error(error_message_name + " must be array, but is `"s + dump_for_error_message(json) + "`"s);
  }
}

inline void requires_boolean(const nlohmann::json& json, const std::string& error_message_name) {
  using namespace std::string_literals;

  if (!json.is_boolean()) {
    throw unmarshal_error(error_message_name + " must be boolean, but is `"s + dump_for_error_message(json) + "`"s);
  }
}

inline void requires_number(const nlohmann::json& json, const std::string& error_message_name) {
  using namespace std::string_literals;

  if (!json.is_number()) {
    throw unmarshal_error(error_message_name + " must be number, but is `"s + dump_for_error_message(json) + "`"s);
  }
}

inline void requires_object(const nlohmann::json& json, const std::string& error_message_name) {
  using namespace std::string_literals;

  if (!json.is_object()) {
    throw unmarshal_error(error_message_name + " must be object, but is `"s + dump_for_error_message(json) + "`"s);
  }
}

inline void requires_string(const nlohmann::json& json, const std::string& error_message_name) {
  using namespace std::string_literals;

  if (!json.is_string()) {
    throw unmarshal_error(error_message_name + " must be string, but is `"s + dump_for_error_message(json) + "`"s);
  }
}
} // namespace json
} // namespace pqrs
