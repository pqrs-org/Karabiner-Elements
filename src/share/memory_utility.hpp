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

namespace std {
// This definition is required to prevent the following error:
//
// ```
// error: call to deleted constructor of 'gsl::not_null_hash<gsl::not_null<shared_ptr<T>>>'
// ```
template <typename T>
struct hash<gsl::not_null<std::shared_ptr<T>>> final {
  std::size_t operator()(const gsl::not_null<std::shared_ptr<T>>& value) const {
    return std::hash<T>{}(*value);
  }
};
} // namespace std
