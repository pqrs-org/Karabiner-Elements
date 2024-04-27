#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <stdexcept>

namespace pqrs {
namespace json {
class unmarshal_error : public std::runtime_error {
public:
  unmarshal_error(const std::string& message) : std::runtime_error(message) {
  }
};
} // namespace json
} // namespace pqrs
