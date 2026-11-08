#pragma once

// pqrs::osx::autorelease_pool v1.1.0

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "autorelease_pool/impl/impl.h"
#include <functional>

namespace pqrs::osx::autorelease_pool {

// `scoped_autorelease_pool` must be destroyed on the same thread on which it was created.
class scoped_autorelease_pool final {
public:
  scoped_autorelease_pool() noexcept
      : token_(pqrs_osx_autorelease_pool_push()) {
  }

  scoped_autorelease_pool(const scoped_autorelease_pool&) = delete;
  scoped_autorelease_pool& operator=(const scoped_autorelease_pool&) = delete;
  scoped_autorelease_pool(scoped_autorelease_pool&&) = delete;
  scoped_autorelease_pool& operator=(scoped_autorelease_pool&&) = delete;

  ~scoped_autorelease_pool() noexcept {
    pqrs_osx_autorelease_pool_pop(token_);
  }

private:
  void* token_;
};

inline void execute_with_autorelease_pool(const std::function<void()>& function) {
  scoped_autorelease_pool pool;
  function();
}

} // namespace pqrs::osx::autorelease_pool
