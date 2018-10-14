#pragma once

#include "dispatcher.hpp"

namespace krbn {
class dispatcher_utility final {
public:
  static void initialize_dispatchers(void) {
    pqrs::dispatcher::extra::initialize_shared_dispatcher();
  }

  static void terminate_dispatchers(void) {
    pqrs::dispatcher::extra::terminate_shared_dispatcher();
  }
};
} // namespace krbn
