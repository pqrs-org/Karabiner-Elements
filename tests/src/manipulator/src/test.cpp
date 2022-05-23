#include "manipulator_factory_test.hpp"
#include "manipulator_manager_test.hpp"

int main(void) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  run_manipulator_factory_test();
  run_manipulator_manager_test();

  return 0;
}
