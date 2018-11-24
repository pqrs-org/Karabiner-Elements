#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <cctype>

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

inline std::string trim_left_copy(std::string s) {
  trim_left(s);
  return s;
}

inline std::string trim_right_copy(std::string s) {
  trim_right(s);
  return s;
}

static inline std::string trim_copy(std::string s) {
  trim(s);
  return s;
}
} // namespace string
} // namespace pqrs
