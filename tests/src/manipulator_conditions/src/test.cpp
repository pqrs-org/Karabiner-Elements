#include "device_exists_test.hpp"
#include "errors_test.hpp"
#include "manipulator_conditions_test.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  run_errors_test();
  run_manipulator_conditions_test();
  run_device_exists_test();

  return 0;
}
