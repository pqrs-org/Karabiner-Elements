#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utf8cpp/utf8.h>

namespace pqrs {
namespace string {
inline void trim_left(std::string& s) {
  s.erase(s.begin(),
          std::find_if(s.begin(),
                       s.end(),
                       [](int c) {
                         return !std::isspace(c);
                       }));
}

inline void trim_right(std::string& s) {
  s.erase(std::find_if(s.rbegin(),
                       s.rend(),
                       [](int c) {
                         return !std::isspace(c);
                       })
              .base(),
          s.end());
}

inline void trim(std::string& s) {
  trim_left(s);
  trim_right(s);
}

inline std::string trim_left_copy(const std::string_view& s) {
  std::string result(s);
  trim_left(result);
  return result;
}

inline std::string trim_right_copy(const std::string_view& s) {
  std::string result(s);
  trim_right(result);
  return result;
}

inline std::string trim_copy(const std::string_view& s) {
  std::string result(s);
  trim(result);
  return result;
}

inline void trim_invalid_right(std::string& s) {
  auto pos = utf8::find_invalid(s);
  if (pos != std::string::npos) {
    s = s.substr(0, pos);
  }
}

inline std::string trim_invalid_right_copy(const std::string_view& s) {
  std::string result(s);
  trim_invalid_right(result);
  return result;
}
} // namespace string
} // namespace pqrs
