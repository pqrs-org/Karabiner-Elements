#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::dispatcher_client` can be used safely in a multi-threaded environment.

#include "dispatcher/dispatcher.hpp"
#include <memory>

namespace pqrs {
namespace dispatcher {
namespace extra {
class dispatcher_client {
public:
  dispatcher_client(std::weak_ptr<dispatcher> weak_dispatcher) : weak_dispatcher_(weak_dispatcher),
                                                                 object_id_(make_new_object_id()) {
    if (auto d = weak_dispatcher_.lock()) {
      d->attach(object_id_);
    }
  }

  virtual ~dispatcher_client(void) {
    if (auto d = weak_dispatcher_.lock()) {
      if (d->attached(object_id_)) {
        // You must use detach_from_dispatcher explicitly.
        abort();
      }
    }
  }

  void detach_from_dispatcher(const std::function<void(void)>& function) const {
    if (auto d = weak_dispatcher_.lock()) {
      d->detach(object_id_, function);
    }
  }

  void enqueue_to_dispatcher(const std::function<void(void)>& function,
                             std::chrono::milliseconds when = dispatcher::when_immediately()) const {
    if (auto d = weak_dispatcher_.lock()) {
      d->enqueue(object_id_, function, when);
    }
  }

  std::chrono::milliseconds when_now(void) const {
    if (auto d = weak_dispatcher_.lock()) {
      if (auto s = d->get_weak_time_source().lock()) {
        return s->now();
      }
    }

    return dispatcher::when_immediately();
  }

  bool attached(void) {
    if (auto d = weak_dispatcher_.lock()) {
      return d->attached(object_id_);
    }
    return false;
  }

protected:
  std::weak_ptr<dispatcher> weak_dispatcher_;
  object_id object_id_;
};
} // namespace extra
} // namespace dispatcher
} // namespace pqrs
