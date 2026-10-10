#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <xpc/xpc.h>

namespace pqrs::osx::xpc {
class object;
}

namespace pqrs::osx {
[[nodiscard]] inline xpc::object adopt_xpc_object(xpc_object_t value) noexcept;
} // namespace pqrs::osx

namespace pqrs::osx::xpc {

class object final {
public:
  object() noexcept = default;

  // Acquire our own reference to a borrowed object.
  explicit object(xpc_object_t value) noexcept
      : value_(value
                   ? xpc_retain(value)
                   : nullptr) {}

  object(const object& other) noexcept
      : value_(other.value_
                   ? xpc_retain(other.value_)
                   : nullptr) {}

  object(object&& other) noexcept
      : value_(std::exchange(other.value_,
                             nullptr)) {}

  object& operator=(object other) noexcept {
    std::swap(value_,
              other.value_);
    return *this;
  }

  ~object() {
    if (value_) {
      xpc_release(value_);
    }
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return value_ != nullptr;
  }

  [[nodiscard]] xpc_object_t get() const noexcept {
    return value_;
  }

private:
  struct adopt_tag final {};

  object(xpc_object_t value, adopt_tag) noexcept
      : value_(value) {}

  friend object pqrs::osx::adopt_xpc_object(xpc_object_t value) noexcept;

  xpc_object_t value_{nullptr};
};

} // namespace pqrs::osx::xpc

namespace pqrs::osx {

// Take ownership of an existing reference without retaining it.
[[nodiscard]] inline xpc::object adopt_xpc_object(xpc_object_t value) noexcept {
  return xpc::object(value,
                     xpc::object::adopt_tag{});
}

} // namespace pqrs::osx
