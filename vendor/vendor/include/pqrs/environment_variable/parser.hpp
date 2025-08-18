#pragma once

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <cctype>
#include <cstdlib>
#include <optional>
#include <pqrs/string.hpp>
#include <string_view>
#include <utility>

namespace pqrs {
namespace environment_variable {
namespace parser {

enum class quote_kind {
  none,
  single,
  dquote
};

inline bool env_name_start(unsigned char c) {
  return std::isalpha(c) || c == '_';
}

inline bool env_name_char(unsigned char c) {
  return std::isalnum(c) || c == '_';
}

// Scan an environment variable name from s[pos]; return end index [pos, end).
// If the first character is invalid, return std::nullopt.
inline std::optional<size_t> scan_env_name(std::string_view s,
                                           size_t pos) {
  if (pos >= s.size() || !env_name_start(static_cast<unsigned char>(s[pos]))) {
    return std::nullopt;
  }

  size_t j = pos + 1;
  while (j < s.size() && env_name_char(static_cast<unsigned char>(s[j]))) {
    ++j;
  }

  return j;
}

// Check whether the whole string is a valid environment variable name.
inline bool is_valid_env_name(std::string_view name) {
  if (auto end = scan_env_name(name, 0)) {
    return (*end == name.size());
  }

  return false;
}

//
// Quote / comment / expansion
//

// Strip surrounding quotes only and return both the inner string and the quote kind.
inline std::pair<std::string, quote_kind> strip_quotes_no_decode(const std::string& s) {
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    return {
        s.substr(1, s.size() - 2),
        quote_kind::dquote,
    };
  }

  if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
    // Inside single quotes, we will not do any processing later.
    return {
        s.substr(1, s.size() - 2),
        quote_kind::single,
    };
  }

  return {
      s,
      quote_kind::none,
  };
}

// Remove end-of-line comments in a line: delete from the first unescaped,
// out-of-quote '#' to the end of string. Then trim.
inline void strip_eol_comment_inplace(std::string& s) {
  bool in_single_quote = false;
  bool in_double_quote = false;
  size_t backslashes = 0;

  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];

    if (!in_single_quote && c == '"') {
      if (backslashes % 2 == 0) {
        in_double_quote = !in_double_quote;
      }
      backslashes = 0;
      continue;
    }

    if (!in_double_quote && c == '\'') {
      // Single quotes are not escapable; always toggle.
      in_single_quote = !in_single_quote;
      backslashes = 0;
      continue;
    }

    if (!in_single_quote && c == '\\') {
      ++backslashes;
      continue;
    }

    if (!in_single_quote &&
        !in_double_quote &&
        c == '#' &&
        backslashes % 2 == 0 // Prevent “\#” from being interpreted as a comment.
    ) {
      s.erase(i);
      break;
    }

    backslashes = 0;
  }

  pqrs::string::trim(s);
}

// Decode backslash escapes and expand $VAR / ${VAR}.
// - '\$' prevents expansion (odd/even rule on preceding backslashes).
// - Undefined variables expand to an empty string.
// - Inside single quotes, nothing is processed.
inline std::string parse_value_with_expansion(const std::string& src,
                                              quote_kind qk) {
  if (qk == quote_kind::single) {
    return src;
  }

  std::string out;
  out.reserve(src.size());
  size_t i = 0;

  while (i < src.size()) {
    char c = src[i];

    // Handle sequences of backslashes followed by '$' using parity (odd = suppress).
    if (c == '\\') {
      size_t j = i;
      while (j < src.size() && src[j] == '\\') {
        ++j;
      }

      if (j < src.size() && src[j] == '$') {
        size_t backslashes = j - i;
        out.append(backslashes / 2, '\\');

        if (backslashes % 2 == 1) {
          // Odd: '\$' → output literal '$'
          out.push_back('$');
          i = j + 1;
          continue;
        } else {
          // Even: fall through to '$' handling below
          i = j;
        }
      } else {
        // Regular backslash escapes
        if (i + 1 < src.size()) {
          char n = src[i + 1];
          switch (n) {
            case 'n':
              out.push_back('\n');
              break;
            case 'r':
              out.push_back('\r');
              break;
            case 't':
              out.push_back('\t');
              break;
            case '\\':
              out.push_back('\\');
              break;
            case '"':
              out.push_back('"');
              break;
            case '\'':
              out.push_back('\'');
              break;
            default:
              out.push_back(n);
              break; // unknown: drop '\' and keep next
          }
          i += 2;
          continue;
        } else {
          // Trailing single backslash
          out.push_back('\\');
          ++i;
          continue;
        }
      }
    }

    // '$' expansion
    if (src[i] == '$') {
      // ${VARNAME}
      if (i + 1 < src.size() && src[i + 1] == '{') {
        size_t j = i + 2;
        if (auto end = scan_env_name(src,
                                     j)) {
          j = *end;
          if (j < src.size() && src[j] == '}') {
            auto name = src.substr(i + 2, j - (i + 2));
            if (const char* v = std::getenv(name.c_str())) {
              out += v;
            }

            i = j + 1;
            continue;
          }
        }

        // Malformed: treat '$' literally.
        out.push_back('$');
        ++i;
        continue;
      }

      // $VARNAME
      if (auto end = scan_env_name(src, i + 1)) {
        size_t j = *end;
        auto name = src.substr(i + 1, j - (i + 1));
        if (const char* v = std::getenv(name.c_str())) {
          out += v;
        }

        i = j;
        continue;
      }

      // Lone '$'
      out.push_back('$');
      ++i;
      continue;
    }

    // Ordinary character
    out.push_back(c);
    ++i;
  }

  return out;
}

// Parse a single line into (key, value). Return std::nullopt for ignorable lines.
inline std::optional<std::pair<std::string, std::string>> parse_line(std::string_view line_view) {
  std::string line(line_view);
  strip_eol_comment_inplace(line);
  pqrs::string::trim(line);

  if (line.empty()) {
    return std::nullopt;
  }

  auto eq = line.find('=');
  if (eq == std::string::npos) {
    return std::nullopt;
  }

  auto key = line.substr(0, eq);
  auto value_raw = line.substr(eq + 1);
  pqrs::string::trim(key);
  pqrs::string::trim(value_raw);

  if (!is_valid_env_name(key)) {
    return std::nullopt;
  }

  auto [inner, qk] = strip_quotes_no_decode(value_raw);
  auto value = parse_value_with_expansion(inner, qk);
  return std::make_pair(key, value);
}

} // namespace parser
} // namespace environment_variable
} // namespace pqrs
