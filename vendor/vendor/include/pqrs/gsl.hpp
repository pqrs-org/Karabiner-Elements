#pragma once

// pqrs::gsl v1.1

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <gsl/gsl>

namespace pqrs {

template <typename T>
using not_null_shared_ptr_t = gsl::not_null<std::shared_ptr<T>>;

template <typename T>
inline std::shared_ptr<T> unwrap_not_null(not_null_shared_ptr_t<T> p) {
  return p.get();
}

template <typename T>
inline std::weak_ptr<T> make_weak(not_null_shared_ptr_t<T> p) {
  return std::weak_ptr<T>(p.get());
}

} // namespace pqrs

namespace std {
// This definition is required to prevent the following error:
//
// ```
// error: call to deleted constructor of 'gsl::not_null_hash<gsl::not_null<shared_ptr<T>>>'
// ```
template <typename T>
struct hash<pqrs::not_null_shared_ptr_t<T>> final {
  std::size_t operator()(const pqrs::not_null_shared_ptr_t<T>& value) const {
    return std::hash<T>{}(*value);
  }
};
} // namespace std
