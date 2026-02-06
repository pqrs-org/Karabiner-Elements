#include "device_exists_test.hpp"
#include "device_test.hpp"
#include "errors_test.hpp"
#include "manipulator_conditions_test.hpp"
#include "run_loop_thread_utility.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  run_errors_test();
  run_manipulator_conditions_test();
  run_device_exists_test();
  run_device_test();

  return 0;
}
