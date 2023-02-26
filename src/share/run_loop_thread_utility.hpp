#pragma once

#include <pqrs/cf/run_loop_thread.hpp>

namespace krbn {
class run_loop_thread_utility {
public:
  class scoped_run_loop_thread_manager final {
  public:
    scoped_run_loop_thread_manager(void) {
      pqrs::cf::run_loop_thread::extra::initialize_shared_run_loop_thread();

      get_power_management_run_loop_thread() = std::make_shared<pqrs::cf::run_loop_thread>();
    }

    ~scoped_run_loop_thread_manager(void) {
      if (get_power_management_run_loop_thread()) {
        get_power_management_run_loop_thread()->terminate();
        get_power_management_run_loop_thread() = nullptr;
      }

      pqrs::cf::run_loop_thread::extra::terminate_shared_run_loop_thread();
    }
  };

  static std::shared_ptr<scoped_run_loop_thread_manager> initialize_shared_run_loop_thread(void) {
    return std::make_shared<scoped_run_loop_thread_manager>();
  }

  // pqrs::osx::iokit_power_management::monitor blocks in the system_will_sleep process until ready to sleep.
  // To allow shared_run_loop_thread to be used to prepare for sleep, pqrs::osx::iokit_power_management::monitor uses an independent run_loop_thread.
  static std::shared_ptr<pqrs::cf::run_loop_thread>& get_power_management_run_loop_thread(void) {
    static std::shared_ptr<pqrs::cf::run_loop_thread> p;
    return p;
  }
}; // namespace run_loop_thread_utility
} // namespace krbn
