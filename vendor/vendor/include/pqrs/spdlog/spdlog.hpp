#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <cctype>
#include <cstdint>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <string_view>

namespace pqrs::spdlog {
[[nodiscard]] inline const char* get_pattern() noexcept {
  return "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";
}

[[nodiscard]] inline std::optional<uint64_t> find_date_number(std::string_view line) noexcept {
  // line == "[2016-09-22 20:18:37.649] [info] [<name>] <message>"
  // return 20160922201837649

  constexpr std::string_view timestamp_format = "[0000-00-00 00:00:00.000]";

  if (line.size() < timestamp_format.size()) {
    return std::nullopt;
  }

  if (line[0] != '[' ||
      line[5] != '-' ||
      line[8] != '-' ||
      line[11] != ' ' ||
      line[14] != ':' ||
      line[17] != ':' ||
      line[20] != '.' ||
      line[24] != ']') {
    return std::nullopt;
  }

  uint64_t result = 0;

  for (auto index : {1, 2, 3, 4,
                     6, 7,
                     9, 10,
                     12, 13,
                     15, 16,
                     18, 19,
                     21, 22, 23}) {
    if (!std::isdigit(static_cast<unsigned char>(line[index]))) {
      return std::nullopt;
    }

    result = result * 10 + static_cast<uint64_t>(line[index] - '0');
  }

  return result;
}

[[nodiscard]] inline std::optional<uint64_t> make_sort_key(std::string_view line) noexcept {
  return find_date_number(line);
}

[[nodiscard]] inline std::optional<::spdlog::level::level_enum> find_level(std::string_view line) {
  constexpr std::string_view prefix = "[0000-00-00 00:00:00.000] [";
  auto front = prefix.size();
  if (line.size() <= front) {
    return std::nullopt;
  }

  for (int i = 0; i < ::spdlog::level::off; ++i) {
    auto level = ::spdlog::level::level_enum(i);
    auto level_name = ::spdlog::level::to_string_view(level);

    std::string level_name_string(level_name.data(), level_name.size());
    level_name_string += "]";

    if (line.substr(front).starts_with(level_name_string)) {
      return level;
    }
  }

  return std::nullopt;
}

[[nodiscard]] inline ::spdlog::filename_t make_rotated_file_path(::spdlog::filename_t file_path) {
  return ::spdlog::sinks::rotating_file_sink<std::mutex>::calc_filename(file_path, 1);
}
} // namespace pqrs::spdlog
