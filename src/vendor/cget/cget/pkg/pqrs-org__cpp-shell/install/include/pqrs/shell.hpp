#pragma once

// pqrs::shell v1.1

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/string.hpp>
#include <sstream>

namespace pqrs {
namespace shell {
inline std::string make_background_command_string(const std::string& command) {
  auto trimmed_command = string::trim_copy(command);
  if (trimmed_command.empty()) {
    return "";
  }

  if (trimmed_command.back() != '&') {
    trimmed_command += " &";
  }

  return trimmed_command;
}
} // namespace shell
} // namespace pqrs
