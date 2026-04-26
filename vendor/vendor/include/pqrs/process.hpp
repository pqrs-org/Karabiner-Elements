#pragma once

// pqrs::process v2.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "process/execute.hpp"
#include "process/process.hpp"
#include <cstdlib>
#include <optional>
#include <string>

namespace pqrs::process {

inline std::optional<int> system(const std::string& command) {
  auto status = std::system(command.c_str());

  if (status == -1 || !WIFEXITED(status)) {
    return std::nullopt;
  }

  // `system` itself may return `127` when shell execution fails,
  // but the executed process may also return `127`.
  // As long as we use `std::system`, there is no reliable way to
  // distinguish these cases, so `127` is returned as-is.
  return WEXITSTATUS(status);
}

} // namespace pqrs::process
