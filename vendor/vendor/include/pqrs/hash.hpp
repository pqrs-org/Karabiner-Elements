#pragma once

// pqrs::hash v2.2.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

namespace pqrs::hash {

template <typename T>
inline void combine(std::size_t& seed,
                    const T& value) noexcept(noexcept(std::hash<T>{}(value)));

template <typename A, typename B>
inline void combine(std::size_t& seed,
                    const std::pair<A, B>& value);

template <typename T, typename Allocator>
inline void combine(std::size_t& seed,
                    const std::vector<T, Allocator>& value);

template <typename... Ts>
inline void combine(std::size_t& seed,
                    const std::variant<Ts...>& value);

template <typename T>
inline void combine(std::size_t& seed,
                    const T& value) noexcept(noexcept(std::hash<T>{}(value))) {
  seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename A, typename B>
inline void combine(std::size_t& seed,
                    const std::pair<A, B>& value) {
  combine(seed,
          value.first);
  combine(seed,
          value.second);
}

template <typename T, typename Allocator>
inline void combine(std::size_t& seed,
                    const std::vector<T, Allocator>& value) {
  combine(seed,
          value.size());

  for (const auto& v : value) {
    if constexpr (std::same_as<T, bool>) {
      combine(seed,
              static_cast<bool>(v));
    } else {
      combine(seed,
              v);
    }
  }
}

template <typename... Ts>
inline void combine(std::size_t& seed,
                    const std::variant<Ts...>& value) {
  combine(seed,
          value.index());

  if (!value.valueless_by_exception()) {
    std::visit(
        [&seed](const auto& v) {
          combine(seed,
                  v);
        },
        value);
  }
}

} // namespace pqrs::hash
