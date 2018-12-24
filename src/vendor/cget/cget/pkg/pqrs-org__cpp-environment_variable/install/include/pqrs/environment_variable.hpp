#pragma once

// pqrs::environment_variable v1.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
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
} // namespace environment_variable
} // namespace pqrs
