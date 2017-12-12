#pragma once

#include "boost_defs.hpp"

#include <boost/optional.hpp>
#include <json/json.hpp>

namespace krbn {
class json_utility final {
public:
  template <typename T>
  static boost::optional<T> find_optional(const nlohmann::json& json,
                                          const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      try {
        return it->get<T>();
      } catch (std::exception&) {
      }
    }

    return boost::none;
  }

  static const nlohmann::json* find_array(const nlohmann::json& json,
                                          const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_array()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static const nlohmann::json* find_object(const nlohmann::json& json,
                                           const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_object()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static nlohmann::json find_copy(const nlohmann::json& json,
                                  const std::string& key,
                                  const nlohmann::json& fallback_value) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      return *it;
    }
    return fallback_value;
  }
};
} // namespace krbn
