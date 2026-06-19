#pragma once

// pqrs::osx::iokit_iterator v1.2.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/iokit_object_ptr.hpp>
#include <utility>

namespace pqrs::osx {
class iokit_iterator final {
public:
  //
  // Constructors
  //

  iokit_iterator() noexcept
      : iokit_iterator(IO_OBJECT_NULL) {
  }

  explicit iokit_iterator(io_iterator_t iterator) noexcept
      : iterator_(iterator) {
  }

  explicit iokit_iterator(const iokit_object_ptr& iterator) noexcept
      : iterator_(iterator) {
  }

  explicit iokit_iterator(iokit_object_ptr&& iterator) noexcept
      : iterator_(std::move(iterator)) {
  }

  //
  // Methods
  //

  [[nodiscard]] const iokit_object_ptr& get() const noexcept {
    return iterator_;
  }

  [[nodiscard]] iokit_object_ptr next() const noexcept {
    iokit_object_ptr result;

    if (iterator_) {
      result = adopt_iokit_object_ptr(IOIteratorNext(*iterator_));
    }

    return result;
  }

  void reset() const noexcept {
    if (iterator_) {
      IOIteratorReset(*iterator_);
    }
  }

  [[nodiscard]] bool valid() const noexcept {
    if (iterator_) {
      return IOIteratorIsValid(*iterator_);
    }
    return false;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return valid();
  }

private:
  iokit_object_ptr iterator_;
};

// Wrap an IOKit iterator returned with a +1 retain count.
[[nodiscard]] inline iokit_iterator adopt_iokit_iterator(io_iterator_t iterator) noexcept {
  return iokit_iterator(adopt_iokit_object_ptr(iterator));
}
} // namespace pqrs::osx
