#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/frontmost_application_monitor/application.hpp>

namespace pqrs {
namespace osx {
namespace frontmost_application_monitor {
inline std::size_t hash_value(const application& value) {
  return std::hash<application>{}(value);
}
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
