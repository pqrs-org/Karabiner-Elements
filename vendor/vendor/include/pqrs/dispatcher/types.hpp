#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <chrono>

namespace pqrs::dispatcher {
using duration = std::chrono::milliseconds;
using time_point = std::chrono::time_point<std::chrono::system_clock, duration>;
} // namespace pqrs::dispatcher
