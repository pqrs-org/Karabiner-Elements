#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::hardware_time_source` can be used safely in a multi-threaded environment.
// `pqrs::dispatcher::pseudo_time_source` can be used safely in a multi-threaded environment.

#include <chrono>
#include <mutex>

namespace pqrs {
namespace dispatcher {
class time_source {
public:
  virtual std::chrono::milliseconds now(void) = 0;
};

class hardware_time_source final : public time_source {
public:
  virtual std::chrono::milliseconds now(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
  };
};

class pseudo_time_source final : public time_source {
public:
  pseudo_time_source(void) : now_(0) {
  }

  virtual std::chrono::milliseconds now(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    return now_;
  }

  void set_now(std::chrono::milliseconds value) {
    std::lock_guard<std::mutex> lock(mutex_);

    now_ = value;
  }

private:
  std::chrono::milliseconds now_;
  mutable std::mutex mutex_;
};
} // namespace dispatcher
} // namespace pqrs
