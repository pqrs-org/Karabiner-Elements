#pragma once

// pqrs::osx::iokit_iterator v1.1

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/osx/iokit_object_ptr.hpp>

namespace pqrs {
namespace osx {
class iokit_iterator final {
public:
  //
  // Constructors
  //

  iokit_iterator(void) : iokit_iterator(IO_OBJECT_NULL) {
  }

  explicit iokit_iterator(io_iterator_t iterator) : iterator_(iterator) {
  }

  explicit iokit_iterator(const iokit_object_ptr& iterator) : iterator_(iterator) {
  }

  //
  // Methods
  //

  const iokit_object_ptr& get(void) const {
    return iterator_;
  }

  iokit_object_ptr next(void) const {
    iokit_object_ptr result;

    if (iterator_) {
      auto it = IOIteratorNext(*iterator_);
      if (it) {
        result = iokit_object_ptr(it);
        IOObjectRelease(it);
      }
    }

    return result;
  }

  void reset(void) const {
    if (iterator_) {
      IOIteratorReset(*iterator_);
    }
  }

  bool valid(void) const {
    if (iterator_) {
      return IOIteratorIsValid(*iterator_);
    }
    return false;
  }

  operator bool(void) const {
    return valid();
  }

private:
  iokit_object_ptr iterator_;
};
} // namespace osx
} // namespace pqrs
