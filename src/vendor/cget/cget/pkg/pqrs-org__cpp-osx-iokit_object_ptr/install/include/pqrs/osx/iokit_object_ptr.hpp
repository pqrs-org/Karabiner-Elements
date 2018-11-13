#pragma once

// pqrs::osx::iokit_object_ptr v2.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/IOKitLib.h>

namespace pqrs {
namespace osx {
class iokit_object_ptr final {
public:
  iokit_object_ptr(void) : iokit_object_ptr(IO_OBJECT_NULL) {
  }

  iokit_object_ptr(io_object_t p) : p_(p) {
    if (p_) {
      IOObjectRetain(p_);
    }
  }

  iokit_object_ptr(const iokit_object_ptr& other) : p_(IO_OBJECT_NULL) {
    *this = other;
  }

  iokit_object_ptr& operator=(const iokit_object_ptr& other) {
    auto old = p_;

    p_ = other.p_;
    if (p_) {
      IOObjectRetain(p_);
    }

    if (old) {
      IOObjectRelease(old);
    }

    return *this;
  }

  ~iokit_object_ptr(void) {
    reset();
  }

  const io_object_t& get(void) const {
    return p_;
  }

  io_object_t& get(void) {
    return const_cast<io_object_t&>((static_cast<const iokit_object_ptr&>(*this)).get());
  }

  void reset(void) {
    if (p_) {
      IOObjectRelease(p_);
      p_ = IO_OBJECT_NULL;
    }
  }

  operator bool(void) const {
    return p_ != IO_OBJECT_NULL;
  }

  const io_object_t& operator*(void)const {
    return p_;
  }

  io_object_t& operator*(void) {
    return const_cast<io_object_t&>(*(static_cast<const iokit_object_ptr&>(*this)));
  }

private:
  io_object_t p_;
};
} // namespace osx
} // namespace pqrs
