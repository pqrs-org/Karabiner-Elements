#pragma once

#include <gsl/gsl>
#include <memory>

namespace krbn {
namespace memory_utility {

template <typename T>
inline std::shared_ptr<T> unwrap_not_null(gsl::not_null<std::shared_ptr<T>> p) {
  return p.get();
}

template <typename T>
inline std::weak_ptr<T> make_weak(gsl::not_null<std::shared_ptr<T>> p) {
  return std::weak_ptr<T>(p.get());
}

} // namespace memory_utility
} // namespace krbn
