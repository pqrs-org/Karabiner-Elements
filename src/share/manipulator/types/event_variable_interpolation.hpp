#pragma once

// Expansion of `${name}` placeholders in the string parameters of `to` events
// (`shell_command` and `set_notification_message.text`).
//
// This header intentionally depends only on the standard library so that it can be unit tested in isolation.

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace krbn::manipulator::event_variable_interpolation {

using variables = std::vector<std::pair<std::string_view, std::string_view>>;

// A value is substituted only if it consists solely of ASCII letters, digits and `_`.
//
// `shell_command` is passed to `/bin/sh -c`, so a value containing whitespace or shell metacharacters
// could change the meaning of the command. Key names are always in this character set,
// so this check never rejects a real key name; it only guards against unexpected values.
[[nodiscard]] inline bool is_safe_value(std::string_view value) {
  if (value.empty()) {
    return false;
  }

  for (unsigned char c : value) {
    if (!std::isalnum(c) && c != '_') {
      return false;
    }
  }

  return true;
}

// Replace each `${name}` in `text` with the value of `name` in `variables`.
//
// A placeholder is left exactly as written when
// - `name` is not in `variables` (so existing shell variables such as `${HOME}` keep working), or
// - the value is not safe (see `is_safe_value`), or
// - the placeholder is not terminated by `}`.
[[nodiscard]] inline std::string expand(const std::string& text,
                                        const variables& variables) {
  std::string result;
  result.reserve(text.size());

  std::size_t pos = 0;
  while (pos < text.size()) {
    auto open = text.find("${", pos);
    if (open == std::string::npos) {
      result.append(text, pos, std::string::npos);
      break;
    }

    result.append(text, pos, open - pos);

    auto close = text.find('}', open + 2);
    if (close == std::string::npos) {
      // Unterminated placeholder: copy the rest as-is.
      result.append(text, open, std::string::npos);
      break;
    }

    std::string_view name(text.data() + open + 2, close - (open + 2));

    bool replaced = false;
    for (const auto& [variable_name, value] : variables) {
      if (variable_name == name && is_safe_value(value)) {
        result.append(value);
        replaced = true;
        break;
      }
    }

    if (!replaced) {
      result.append(text, open, close + 1 - open);
    }

    pos = close + 1;
  }

  return result;
}

} // namespace krbn::manipulator::event_variable_interpolation
