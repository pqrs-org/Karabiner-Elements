#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::dispatcher_client` can be used safely in a multi-threaded environment.

#include "../dispatcher.hpp"
#include "shared_dispatcher.hpp"
#include <memory>

namespace pqrs::dispatcher::extra {
class dispatcher_client {
public:
  dispatcher_client(std::weak_ptr<dispatcher> weak_dispatcher = get_shared_dispatcher()) : weak_dispatcher_(weak_dispatcher),
                                                                                           object_id_(make_new_object_id()) {
    if (auto d = weak_dispatcher_.lock()) {
      // `attach` may fail if the dispatcher is terminating or already terminated.
      d->attach(object_id_);
    }
  }

  virtual ~dispatcher_client() {
    if (auto d = weak_dispatcher_.lock()) {
      if (d->attached(object_id_)) {
        // You must use detach_from_dispatcher explicitly.
        abort();
      }
    }
  }

  void detach_from_dispatcher() const {
    if (auto d = weak_dispatcher_.lock()) {
      d->detach(object_id_);
    }
  }

  void detach_from_dispatcher(std::function<void()> function) const {
    if (auto d = weak_dispatcher_.lock()) {
      d->detach(object_id_, function);
    }
  }

  // Returns false if the dispatcher is unavailable, terminating, already
  // terminated, or this client is no longer attached.
  bool enqueue_to_dispatcher(std::function<void()> function,
                             time_point when = dispatcher::when_immediately()) const {
    if (auto d = weak_dispatcher_.lock()) {
      return d->enqueue(object_id_, function, when);
    }

    return false;
  }

  time_point when_now() const {
    if (auto d = weak_dispatcher_.lock()) {
      if (auto s = d->lock_weak_time_source()) {
        return s->now();
      }
    }

    return dispatcher::when_immediately();
  }

  bool attached() {
    if (auto d = weak_dispatcher_.lock()) {
      return d->attached(object_id_);
    }
    return false;
  }

  bool dispatcher_thread() const {
    if (auto d = weak_dispatcher_.lock()) {
      return d->dispatcher_thread();
    }
    return false;
  }

protected:
  std::weak_ptr<dispatcher> weak_dispatcher_;
  object_id object_id_;
};
} // namespace pqrs::dispatcher::extra
