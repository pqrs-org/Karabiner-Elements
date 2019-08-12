#pragma once

// pqrs::gcd v1.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <atomic>
#include <cstring>
#include <dispatch/dispatch.h>

namespace pqrs {
namespace gcd {
namespace impl {
inline static std::atomic<int> running_on_main_queue_marker_count = 0;
}

class scoped_running_on_main_queue_marker final {
public:
  scoped_running_on_main_queue_marker(void) {
    ++impl::running_on_main_queue_marker_count;
  }

  ~scoped_running_on_main_queue_marker(void) {
    --impl::running_on_main_queue_marker_count;
  }
};

inline bool running_on_main_queue(void) {
  if (impl::running_on_main_queue_marker_count > 0) {
    return true;
  }

  auto main_queue_label = dispatch_queue_get_label(dispatch_get_main_queue());
  auto current_queue_label = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL);

  return strcmp(main_queue_label, current_queue_label) == 0;
}

inline void dispatch_sync_on_main_queue(void (^block)(void)) {
  if (running_on_main_queue()) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(),
                  block);
  }
}
} // namespace gcd
} // namespace pqrs
