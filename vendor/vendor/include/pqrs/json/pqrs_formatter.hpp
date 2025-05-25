#pragma once

// (C) Copyright Takayama Fumihiko 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
#include <sstream>
#include <unordered_set>

namespace pqrs {
namespace json {
namespace pqrs_formatter {

// The custom JSON formatter follows these rules:
//
// - Arrays are kept on a single line. However, they are spread across multiple lines in the following cases:
//   - If the size is 2 or more and they contain arrays or objects as values.
//   - If they contain multi-line values.
// - Objects with only one key and a single-line value are kept on a single line.
//
// ```json
// {
//     "bool": true,
//     "double": 123.456,
//     "int": 42,
//     "force_multi_line_array": [
//         "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
//         "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
//         "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.",
//         "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
//     ],
//     "multi_line_array1": [
//         [1, 2, 3, 4],
//         [5, 6, 7, 8]
//     ],
//     "multi_line_array2": [
//         { "key": 1 },
//         { "key": 2 }
//     ],
//     "multi_line_object1": {
//         "key1": 1,
//         "key2": 2
//     },
//     "multi_line_object2": {
//         "key": {
//             "key1": 1,
//             "key2": 2
//         }
//     },
//     "null": null,
//     "single_line_array1": [],
//     "single_line_array2": [1, 2, 3, 4],
//     "single_line_array3": [[1, 2, 3, 4]],
//     "single_line_array4": [{ "key": "value" }],
//     "single_line_object1": {},
//     "single_line_object2": { "key": "value" },
//     "single_line_object3": { "key": [1, 2, 3, 4] },
//     "single_line_object4": { "key1": { "key2": { "key3": "value3" } } }
// }
// ```

struct options {
  std::optional<int> indent_size;
  std::optional<nlohmann::json::error_handler_t> error_handler;
  std::unordered_set<std::string> force_multi_line_array_object_keys;
};

namespace impl {

template <typename T>
inline bool multi_line(const T& json,
                       const options& options,
                       std::optional<std::string> parent_object_key) {
  if (json.is_object()) {
    if (json.size() == 0) {
      return false;
    }

    if (json.size() == 1) {
      return multi_line(json.begin().value(),
                        options,
                        json.begin().key());
    }

    if (json.size() > 1) {
      return true;
    }

  } else if (json.is_array()) {
    if (auto key = parent_object_key) {
      if (options.force_multi_line_array_object_keys.contains(*key)) {
        return true;
      }
    }

    if (json.size() == 0) {
      return false;
    }

    if (json.size() == 1) {
      return multi_line(json[0],
                        options,
                        std::nullopt);
    }

    for (const auto& j : json) {
      if (j.is_object() || j.is_array()) {
        return true;
      }
    }
  }

  return false;
}

inline void indent(std::ostringstream& ss,
                   const options& options,
                   int indent_level) {
  for (int i = 0; i < options.indent_size.value_or(4) * indent_level; ++i) {
    ss << ' ';
  }
}

template <typename T>
inline void format(std::ostringstream& ss,
                   const T& json,
                   const options& options,
                   std::optional<std::string> parent_object_key,
                   int indent_level) {
  if (json.is_object()) {
    if (!multi_line(json, options, parent_object_key)) {
      //
      // Single-line object
      //

      if (json.empty()) {
        ss << "{}";

      } else {

        ss << "{ ";

        ss << std::quoted(json.begin().key()) << ": ";

        format(ss,
               json.begin().value(),
               options,
               json.begin().key(),
               indent_level + 1);

        ss << " }";
      }

    } else {
      //
      // Multi-line object
      //

      ss << "{\n";

      bool first = true;
      for (const auto& [k, v] : json.items()) {
        if (!first) {
          ss << ",\n";
        }
        first = false;

        indent(ss,
               options,
               indent_level + 1);

        ss << std::quoted(k) << ": ";
        format(ss,
               v,
               options,
               k,
               indent_level + 1);
      }

      ss << '\n';

      indent(ss,
             options,
             indent_level);

      ss << '}';
    }
  } else if (json.is_array()) {
    if (!multi_line(json, options, parent_object_key)) {
      //
      // Single-line array
      //

      ss << '[';

      bool first = true;
      for (const auto& v : json) {
        if (!first) {
          ss << ", ";
        }
        first = false;

        format(ss,
               v,
               options,
               std::nullopt,
               indent_level + 1);
      }

      ss << ']';

    } else {
      //
      // Multi-line array
      //

      ss << "[\n";

      bool first = true;
      for (const auto& v : json) {
        if (!first) {
          ss << ",\n";
        }
        first = false;

        indent(ss,
               options,
               indent_level + 1);

        format(ss,
               v,
               options,
               std::nullopt,
               indent_level + 1);
      }

      ss << '\n';

      indent(ss,
             options,
             indent_level);

      ss << ']';
    }
  } else {
    ss << json.dump(0,
                    ' ',
                    false,
                    options.error_handler.value_or(nlohmann::json::error_handler_t::strict));
  }
}

} // namespace impl

template <typename T>
inline std::string format(const T& json,
                          const options& options) {
  std::ostringstream ss;
  impl::format(ss, json, options, std::nullopt, 0);
  return ss.str();
}

} // namespace pqrs_formatter
} // namespace json
} // namespace pqrs
