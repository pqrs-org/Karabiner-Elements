#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <chrono>

namespace pqrs {
namespace dispatcher {
typedef std::chrono::milliseconds duration;
typedef std::chrono::time_point<std::chrono::system_clock, duration> time_point;
} // namespace dispatcher
} // namespace pqrs
