#pragma once

// pqrs::environment_variable v1.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "environment_variable/parser.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>

namespace pqrs {
namespace environment_variable {

inline std::optional<std::string> find(const std::string& name) {
  if (const char* p = std::getenv(name.c_str())) {
    return p;
  }
  return std::nullopt;
}

inline void load_environment_variables_from_file(const std::filesystem::path& path,
                                                 const std::function<void(std::string_view name, std::string_view value)>& callback) {
  std::ifstream in(path);
  if (in) {
    std::string line;
    while (std::getline(in, line)) {
      auto kv = parser::parse_line(line);
      if (!kv) continue;

      auto overwrite = 1;
      if (setenv(kv->first.c_str(), kv->second.c_str(), overwrite) == 0) {
        callback(kv->first, kv->second);
      }
    }
  }
}

} // namespace environment_variable
} // namespace pqrs
