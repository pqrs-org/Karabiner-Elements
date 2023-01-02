#pragma once

#include <pqrs/cf/run_loop_thread.hpp>

namespace krbn {
class run_loop_thread_utility {
public:
  class scoped_run_loop_thread_manager final {
  public:
    scoped_run_loop_thread_manager(void) {
      pqrs::cf::run_loop_thread::extra::initialize_shared_run_loop_thread();
    }

    ~scoped_run_loop_thread_manager(void) {
      pqrs::cf::run_loop_thread::extra::terminate_shared_run_loop_thread();
    }
  };

  static std::shared_ptr<scoped_run_loop_thread_manager> initialize_shared_run_loop_thread(void) {
    return std::make_shared<scoped_run_loop_thread_manager>();
  }

}; // namespace run_loop_thread_utility
} // namespace krbn
