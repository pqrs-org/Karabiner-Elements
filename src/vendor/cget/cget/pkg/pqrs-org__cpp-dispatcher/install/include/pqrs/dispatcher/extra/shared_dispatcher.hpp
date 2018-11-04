#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::shared_dispatcher` can be used safely in a multi-threaded environment.

#include "../dispatcher.hpp"

namespace pqrs {
namespace dispatcher {
namespace extra {
class shared_dispatcher final {
public:
  void initialize(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!time_source_) {
      time_source_ = std::make_shared<hardware_time_source>();
    }

    if (!dispatcher_) {
      dispatcher_ = std::make_shared<dispatcher>(time_source_);
    }
  }

  void terminate(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (dispatcher_) {
      dispatcher_->terminate();
      dispatcher_ = nullptr;
    }

    if (time_source_) {
      time_source_ = nullptr;
    }
  }

  std::shared_ptr<dispatcher> get_dispatcher(void) const {
    return dispatcher_;
  }

  static std::shared_ptr<shared_dispatcher> get_shared_dispatcher(void) {
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

inline void initialize_shared_dispatcher(void) {
  auto p = shared_dispatcher::get_shared_dispatcher();
  p->initialize();
}

inline void terminate_shared_dispatcher(void) {
  auto p = shared_dispatcher::get_shared_dispatcher();
  p->terminate();
}

inline std::shared_ptr<dispatcher> get_shared_dispatcher(void) {
  auto p = shared_dispatcher::get_shared_dispatcher();
  return p->get_dispatcher();
}
} // namespace extra
} // namespace dispatcher
} // namespace pqrs
