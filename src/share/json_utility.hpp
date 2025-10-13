#pragma once

#include "exprtk_utility.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/json.hpp>
#include <spdlog/fmt/fmt.h>

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

// The string is saved as an array of strings if it is multi-line; otherwise, it is saved as a single string.
std::string unmarshal_string(const std::string& key,
                             const nlohmann::json& value) {
  if (value.is_string()) {
    return value.get<std::string>();

  } else if (value.is_array()) {
    std::stringstream ss;

    for (const auto& j : value) {
      if (!j.is_string()) {
        goto error;
      }

      ss << j.get<std::string>() << '\n';
    }

    auto s = ss.str();
    // Remove the last newline.
    if (!s.empty()) {
      s.pop_back();
    }

    return s;
  }

error:
  throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array of string, or string, but is `{1}`",
                                                key,
                                                value));
}

// The string is saved as an array of strings if it is multi-line; otherwise, it is saved as a single string.
nlohmann::json marshal_string(const std::string value) {
  if (value.find('\n') == value.npos) {
    return value;
  }

  std::stringstream ss(value);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(ss, line, '\n')) {
    lines.push_back(line);
  }
  return lines;
}

pqrs::not_null_shared_ptr_t<exprtk_utility::expression_wrapper> unmarshal_expression_string(const std::string& key,
                                                                                            const nlohmann::json& value) {
  auto s = unmarshal_string(key, value);
  auto e = exprtk_utility::compile(s);
  if (std::isnan(e->value())) {
    throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: invalid expression `{1}`", key, value));
  }
  return e;
}

}; // namespace json_utility
} // namespace krbn
