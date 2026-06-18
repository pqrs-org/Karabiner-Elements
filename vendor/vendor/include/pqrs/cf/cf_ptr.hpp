#pragma once

// pqrs::cf::cf_ptr v2.2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <utility>

namespace pqrs::cf {
template <typename T>
class cf_ptr;

template <typename T>
cf_ptr<T> adopt_cf_ptr(T _Nullable p) noexcept;

template <typename T>
class cf_ptr final {
public:
  cf_ptr() noexcept : cf_ptr(nullptr) {
  }

  cf_ptr(T _Nullable p) noexcept : p_(p) {
    if (p_) {
      CFRetain(p_);
    }
  }

  cf_ptr(const cf_ptr& other) noexcept : p_(nullptr) {
    *this = other;
  }

  cf_ptr& operator=(const cf_ptr& other) noexcept {
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

  cf_ptr(cf_ptr&& other) noexcept : p_(std::exchange(other.p_, nullptr)) {
  }

  cf_ptr& operator=(cf_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      p_ = std::exchange(other.p_, nullptr);
    }

    return *this;
  }

  ~cf_ptr() noexcept {
    reset();
  }

  const T& get() const noexcept {
    return p_;
  }

  T& get() noexcept {
    return const_cast<T&>((static_cast<const cf_ptr&>(*this)).get());
  }

  void reset() noexcept {
    if (p_) {
      CFRelease(p_);
      p_ = nullptr;
    }
  }

  operator bool() const noexcept {
    return p_ != nullptr;
  }

  const T& operator*() const noexcept {
    return p_;
  }

  T& operator*() noexcept {
    return const_cast<T&>(*(static_cast<const cf_ptr&>(*this)));
  }

private:
  struct adopt_tag final {
  };

  cf_ptr(T _Nullable p, adopt_tag) noexcept : p_(p) {
  }

  template <typename U>
  friend cf_ptr<U> adopt_cf_ptr(U) noexcept;

  T _Nullable p_;
};

template <typename T>
cf_ptr<T> adopt_cf_ptr(T _Nullable p) noexcept {
  return cf_ptr<T>(p, typename cf_ptr<T>::adopt_tag{});
}
} // namespace pqrs::cf
