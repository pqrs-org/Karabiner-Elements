#pragma once

// `krbn::dispatcher_utility` can be used safely in a multi-threaded environment.

#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace krbn {
class dispatcher_utility final {
public:
  class scoped_dispatcher_manager final {
  public:
    scoped_dispatcher_manager() {
      pqrs::dispatcher::extra::initialize_shared_dispatcher();
    }

    ~scoped_dispatcher_manager() {
      pqrs::dispatcher::extra::terminate_shared_dispatcher();
    }
  };

  [[nodiscard]] static pqrs::not_null_shared_ptr_t<scoped_dispatcher_manager> initialize_dispatchers() {
    return std::make_shared<scoped_dispatcher_manager>();
  }
};
} // namespace krbn
