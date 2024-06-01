#include "core_configuration_test.hpp"
#include "device_test.hpp"
#include "errors_test.hpp"
#include "global_configuration_test.hpp"
#include "machine_specific_test.hpp"

int main(void) {
  run_core_configuration_test();
  run_device_test();
  run_errors_test();
  run_global_configuration_test();
  run_machine_specific_test();
  return 0;
}
