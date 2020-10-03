#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
namespace json_utility {
template <typename T>
inline nlohmann::json parse_jsonc(T&& input) {
  bool allow_exceptions = true;
  bool ignore_comments = true;
  return nlohmann::json::parse(input,
                               nullptr,
                               allow_exceptions,
                               ignore_comments);
}

template <typename T>
inline nlohmann::json parse_jsonc(T first, T last) {
  bool allow_exceptions = true;
  bool ignore_comments = true;
  return nlohmann::json::parse(first,
                               last,
                               nullptr,
                               allow_exceptions,
                               ignore_comments);
}

inline std::string dump(const nlohmann::json& json) {
  return json.dump(4,
                   ' ',
                   false,
                   nlohmann::json::error_handler_t::ignore);
}
}; // namespace json_utility
} // namespace krbn
