#pragma once

// pqrs::osx::iokit_object_ptr v3.2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/IOKitLib.h>
#include <optional>
#include <pqrs/osx/kern_return.hpp>
#include <string>
#include <utility>

namespace pqrs::osx {
class iokit_object_ptr final {
public:
  //
  // Constructors
  //

  iokit_object_ptr() noexcept = default;

  explicit iokit_object_ptr(io_object_t p) noexcept
      : p_(p) {
    if (p_) {
      IOObjectRetain(p_);
    }
  }

  iokit_object_ptr(const iokit_object_ptr& other) noexcept
      : iokit_object_ptr(other.p_) {
  }

  iokit_object_ptr(iokit_object_ptr&& other) noexcept
      : p_(std::exchange(other.p_, IO_OBJECT_NULL)) {
  }

  iokit_object_ptr& operator=(const iokit_object_ptr& other) noexcept {
    if (this != &other) {
      iokit_object_ptr copy(other);
      swap(copy);
    }

    return *this;
  }

  iokit_object_ptr& operator=(iokit_object_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      p_ = std::exchange(other.p_, IO_OBJECT_NULL);
    }

    return *this;
  }

  ~iokit_object_ptr() noexcept {
    reset();
  }

  //
  // Pointer methods
  //

  [[nodiscard]] const io_object_t& get() const noexcept {
    return p_;
  }

  [[nodiscard]] io_object_t& get() noexcept {
    return const_cast<io_object_t&>((static_cast<const iokit_object_ptr&>(*this)).get());
  }

  void reset() noexcept {
    if (p_) {
      IOObjectRelease(p_);
      p_ = IO_OBJECT_NULL;
    }
  }

  void swap(iokit_object_ptr& other) noexcept {
    std::swap(p_, other.p_);
  }

  //
  // Utility methods
  //

  [[nodiscard]] uint32_t kernel_retain_count() const noexcept {
    if (p_) {
      return IOObjectGetKernelRetainCount(p_);
    }

    return 0;
  }

  [[nodiscard]] uint32_t user_retain_count() const noexcept {
    if (p_) {
      return IOObjectGetUserRetainCount(p_);
    }

    return 0;
  }

  [[nodiscard]] bool conforms_to(const io_name_t class_name) const noexcept {
    if (p_) {
      return IOObjectConformsTo(p_, class_name);
    }

    return false;
  }

  [[nodiscard]] std::optional<std::string> class_name() const {
    if (p_) {
      io_name_t name;
      kern_return r = IOObjectGetClass(p_, name);
      if (r) {
        return name;
      }
    }

    return std::nullopt;
  }

  //
  // Operators
  //

  [[nodiscard]] operator bool() const noexcept {
    return p_ != IO_OBJECT_NULL;
  }

  [[nodiscard]] const io_object_t& operator*() const noexcept {
    return get();
  }

  [[nodiscard]] io_object_t& operator*() noexcept {
    return get();
  }

private:
  struct adopt_tag final {
  };

  explicit iokit_object_ptr(io_object_t p, adopt_tag) noexcept
      : p_(p) {
  }

  friend iokit_object_ptr adopt_iokit_object_ptr(io_object_t p) noexcept;

  io_object_t p_{IO_OBJECT_NULL};
};

[[nodiscard]] inline iokit_object_ptr adopt_iokit_object_ptr(io_object_t p) noexcept {
  return iokit_object_ptr(p, iokit_object_ptr::adopt_tag{});
}
} // namespace pqrs::osx
