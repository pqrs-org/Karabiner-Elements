#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <type_safe/strong_typedef.hpp>

namespace pqrs::osx::cg_event {
enum class event_type {
  key_down,
  key_up,
};
} // namespace pqrs::osx::cg_event
