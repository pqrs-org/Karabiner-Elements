#pragma once

#include <nlohmann/json.hpp>
#include <pqrs/json.hpp>

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
inline nlohmann::ordered_json parse_ordered_jsonc(T&& input) {
  bool allow_exceptions = true;
  bool ignore_comments = true;
  return nlohmann::ordered_json::parse(input,
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

template <typename T>
inline std::string dump(const T& json) {
  return pqrs::json::pqrs_formatter::format(
      json,
      {.indent_size = 4,
       .error_handler = nlohmann::json::error_handler_t::ignore,
       .force_multi_line_array_object_keys = {
           "bundle_identifiers",
           "game_pad_stick_horizontal_wheel_formula",
           "game_pad_stick_vertical_wheel_formula",
           "game_pad_stick_x_formula",
           "game_pad_stick_y_formula",
       }});
}
}; // namespace json_utility
} // namespace krbn
