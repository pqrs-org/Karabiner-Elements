#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
#include <pqrs/osx/chrono/absolute_time_point.hpp>

namespace pqrs {
namespace osx {
namespace chrono {
void to_json(nlohmann::json& j, const absolute_time_duration& p) {
  j = type_safe::get(p);
}

void from_json(const nlohmann::json& j, absolute_time_duration& p) {
  try {
    p = absolute_time_duration(j.get<int64_t>());
  } catch (...) {}
}

void to_json(nlohmann::json& j, const absolute_time_point& p) {
  j = type_safe::get(p);
}

void from_json(const nlohmann::json& j, absolute_time_point& p) {
  try {
    p = absolute_time_point(j.get<uint64_t>());
  } catch (...) {}
}
} // namespace chrono
} // namespace osx
} // namespace pqrs
