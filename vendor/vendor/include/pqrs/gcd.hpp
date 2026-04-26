#pragma once

// pqrs::gcd v1.3.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <dispatch/dispatch.h>
#include <pthread.h>

namespace pqrs::gcd {
inline void dispatch_sync_on_main_queue(void (^block)(void)) {
  if (pthread_main_np() != 0) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(),
                  block);
  }
}
} // namespace pqrs::gcd
