#pragma once

// `krbn::dispatcher_utility` can be used safely in a multi-threaded environment.

#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/autorelease_pool.hpp>

namespace krbn {
class dispatcher_utility final {
public:
  class scoped_dispatcher_manager final {
  public:
    scoped_dispatcher_manager() {
      pqrs::dispatcher::extra::initialize_shared_dispatcher(
          pqrs::osx::autorelease_pool::execute_with_autorelease_pool);
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
