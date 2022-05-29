#include "errors_test.hpp"
#include "manipulator_conditions_test.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  run_errors_test();
  run_manipulator_conditions_test();

  return 0;
}
