#pragma once

// pqrs::sign v1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

namespace pqrs {
enum class sign {
  zero,
  plus,
  minus,
};

template <typename T>
inline sign make_sign(T value) {
  if (value > 0) {
    return sign::plus;
  } else if (value < 0) {
    return sign::minus;
  } else {
    return sign::zero;
  }
}
} // namespace pqrs
