#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <spdlog/common.h>
#include <sstream>

namespace pqrs {
namespace spdlog {
inline const char* get_pattern(void) {
  return "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";
}

inline std::optional<uint64_t> make_sort_key(const std::string& line) {
  // line == "[2016-09-22 20:18:37.649] [info] [<name>] <message>"
  // return 20160922201837649

  if (line.size() < strlen("[0000-00-00 00:00:00.000]")) {
    return std::nullopt;
  }

  if (line.empty()) {
    return std::nullopt;
  }

  std::stringstream ss;
  size_t pos = 0;

  if (line[pos++] != '[') {
    return std::nullopt;
  }

  // years

  ss << line[pos++];
  ss << line[pos++];
  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != '-') {
    return std::nullopt;
  }

  // months

  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != '-') {
    return std::nullopt;
  }

  // days

  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != ' ') {
    return std::nullopt;
  }

  // hours

  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != ':') {
    return std::nullopt;
  }

  // minutes

  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != ':') {
    return std::nullopt;
  }

  // seconds

  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != '.') {
    return std::nullopt;
  }

  // milliseconds

  ss << line[pos++];
  ss << line[pos++];
  ss << line[pos++];

  if (line[pos++] != ']') {
    return std::nullopt;
  }

  try {
    return std::stoll(ss.str());
  } catch (...) {
  }

  return std::nullopt;
}

inline std::optional<::spdlog::level::level_enum> find_level(const std::string& line) {
  auto front = strlen("[0000-00-00 00:00:00.000] [");
  if (line.size() <= front) {
    return std::nullopt;
  }

  for (int i = 0; i < ::spdlog::level::off; ++i) {
    auto level = ::spdlog::level::level_enum(i);
    auto level_name = std::string(::spdlog::level::to_c_str(level)) + "]";

    if (line.compare(front, level_name.size(), level_name) == 0) {
      return level;
    }
  }

  return std::nullopt;
}
} // namespace spdlog
} // namespace pqrs
