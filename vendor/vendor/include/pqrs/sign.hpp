#pragma once

#include <concepts>
#include <type_traits>

// pqrs::sign v1.1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

namespace pqrs {
enum class sign {
  zero,
  plus,
  minus,
};

template <typename T>
  requires((std::integral<std::remove_cv_t<T>> && !std::same_as<std::remove_cv_t<T>, bool>) ||
           std::floating_point<std::remove_cv_t<T>>)
[[nodiscard]] constexpr sign make_sign(T value) noexcept {
  if (value > 0) {
    return sign::plus;
  } else if (value < 0) {
    return sign::minus;
  } else {
    return sign::zero;
  }
}
} // namespace pqrs
