#pragma once

// (C) Copyright Takayama Fumihiko 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>

namespace nlohmann {
inline auto format_as(const json& value) { return value.dump(); }
} // namespace nlohmann
