#pragma once

// pqrs::hash v2.1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <functional>

namespace pqrs::hash {

template <typename T>
inline void combine(std::size_t& seed, const T& value) noexcept(noexcept(std::hash<T>{}(value))) {
  seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace pqrs::hash
