#pragma once

// pqrs::cf_ptr v1.3.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>

namespace pqrs {
template <typename T>
class cf_ptr final {
public:
  cf_ptr(void) : cf_ptr(nullptr) {
  }

  cf_ptr(T _Nullable p) : p_(p) {
    if (p_) {
      CFRetain(p_);
    }
  }

  cf_ptr(const cf_ptr& other) : p_(nullptr) {
    *this = other;
  }

  cf_ptr& operator=(const cf_ptr& other) {
    auto old = p_;

    p_ = other.p_;
    if (p_) {
      CFRetain(p_);
    }

    if (old) {
      CFRelease(old);
    }

    return *this;
  }

  ~cf_ptr(void) {
    reset();
  }

  const T& get(void) const {
    return p_;
  }

  T& get(void) {
    return const_cast<T&>((static_cast<const cf_ptr&>(*this)).get());
  }

  void reset(void) {
    if (p_) {
      CFRelease(p_);
      p_ = nullptr;
    }
  }

  operator bool(void) const {
    return p_ != nullptr;
  }

  const T& operator*(void)const {
    return p_;
  }

  T& operator*(void) {
    return const_cast<T&>(*(static_cast<const cf_ptr&>(*this)));
  }

private:
  T _Nullable p_;
};
} // namespace pqrs
