#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::hardware_time_source` can be used safely in a multi-threaded environment.
// `pqrs::dispatcher::pseudo_time_source` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <mutex>

namespace pqrs::dispatcher {
class time_source {
public:
  virtual ~time_source() = default;
  virtual time_point now() = 0;
};

class hardware_time_source final : public time_source {
public:
  time_point now() override {
    return std::chrono::time_point_cast<duration>(std::chrono::system_clock::now());
  }
};

class pseudo_time_source final : public time_source {
public:
  pseudo_time_source() : now_(duration::zero()) {
  }

  time_point now() override {
    std::lock_guard<std::mutex> lock(mutex_);

    return now_;
  }

  void set_now(time_point value) {
    std::lock_guard<std::mutex> lock(mutex_);

    now_ = value;
  }

private:
  time_point now_;
  mutable std::mutex mutex_;
};
} // namespace pqrs::dispatcher
