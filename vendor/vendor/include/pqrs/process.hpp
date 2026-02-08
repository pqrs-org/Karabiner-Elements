#pragma once

// pqrs::process v1.6

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "process/execute.hpp"
#include "process/process.hpp"
#include <cstdlib>
#include <optional>
#include <string_view>

namespace pqrs {
namespace process {

inline std::optional<int> system(std::string_view command) {
  auto status = std::system(command.data());

  if (status == -1) {
    return std::nullopt;
  }

  if (WIFEXITED(status)) {
    auto exit_code = WEXITSTATUS(status);
    if (exit_code == 127) {
      // The execution of the shell failed
      return std::nullopt;
    }

    return exit_code;
  }

  return std::nullopt;
}

} // namespace process
} // namespace pqrs
