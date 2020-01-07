#pragma once

// pqrs::json v1.2

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "json/marshal_error.hpp"
#include "json/unmarshal_error.hpp"
#include <nlohmann/json.hpp>
#include <optional>

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
} // namespace json
} // namespace pqrs
