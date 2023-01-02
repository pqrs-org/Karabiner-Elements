#include "manipulator_factory_test.hpp"
#include "manipulator_manager_test.hpp"
#include "run_loop_thread_utility.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_shared_run_loop_thread();

  run_manipulator_factory_test();
  run_manipulator_manager_test();

  return 0;
}
