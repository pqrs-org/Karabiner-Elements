#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <stdexcept>
#include <string>

namespace pqrs::json {
class marshal_error : public std::runtime_error {
public:
  explicit marshal_error(const std::string& message) : std::runtime_error(message) {
  }
};
} // namespace pqrs::json
