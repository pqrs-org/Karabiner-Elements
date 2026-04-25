#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::shared_dispatcher` can be used safely in a multi-threaded environment.

#include "../dispatcher.hpp"

namespace pqrs::dispatcher::extra {
class shared_dispatcher final {
public:
  void initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!time_source_) {
      time_source_ = std::make_shared<hardware_time_source>();
    }

    if (!dispatcher_) {
      dispatcher_ = std::make_shared<dispatcher>(time_source_);
    }
  }

  void terminate() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (dispatcher_) {
      // Keep `dispatcher_->terminate()` inside the lock in order to serialize
      // `initialize`, `get_dispatcher`, and `terminate`.
      // If we release the lock before waiting for terminate, another thread may
      // create a new shared dispatcher while the previous one is still stopping.
      dispatcher_->terminate();
      dispatcher_.reset();
    }

    if (time_source_) {
      time_source_.reset();
    }
  }

  std::shared_ptr<dispatcher> get_dispatcher() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return dispatcher_;
  }

  static std::shared_ptr<shared_dispatcher> get_shared_dispatcher() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    static std::shared_ptr<shared_dispatcher> p;
    if (!p) {
      p = std::make_shared<shared_dispatcher>();
    }

    return p;
  }

private:
  std::shared_ptr<time_source> time_source_;
  std::shared_ptr<dispatcher> dispatcher_;
  mutable std::mutex mutex_;
};

inline void initialize_shared_dispatcher() {
  auto p = shared_dispatcher::get_shared_dispatcher();
  p->initialize();
}

inline void terminate_shared_dispatcher() {
  auto p = shared_dispatcher::get_shared_dispatcher();
  p->terminate();
}

inline std::shared_ptr<dispatcher> get_shared_dispatcher() {
  auto p = shared_dispatcher::get_shared_dispatcher();
  return p->get_dispatcher();
}
} // namespace pqrs::dispatcher::extra
