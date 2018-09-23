#pragma once

#include "dispatcher/dispatcher.hpp"
#include <memory>

namespace krbn {
namespace dispatcher {
class dispatcher_client {
public:
  dispatcher_client(std::weak_ptr<dispatcher> weak_dispatcher) : weak_dispatcher_(weak_dispatcher),
                                                                 object_id_(make_new_object_id()) {
    if (auto d = weak_dispatcher_.lock()) {
      d->attach(object_id_);
    }
  }

  void detach_from_dispatcher(const std::function<void(void)>& function) {
    if (auto d = weak_dispatcher_.lock()) {
      d->detach(object_id_, function);
    }
  }

  void enqueue_to_dispatcher(const std::function<void(void)>& function) {
    if (auto d = weak_dispatcher_.lock()) {
      d->enqueue(object_id_, function);
    }
  }

protected:
  std::weak_ptr<dispatcher> weak_dispatcher_;
  object_id object_id_;
};
} // namespace dispatcher
} // namespace krbn
