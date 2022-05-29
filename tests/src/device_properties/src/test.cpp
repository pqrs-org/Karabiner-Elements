#include "compare_test.hpp"
#include "device_identifiers_test.hpp"
#include "device_properties_test.hpp"
#include "to_json_test.hpp"

int main(void) {
  run_compare_test();
  run_device_identifiers_test();
  run_device_properties_test();
  run_to_json_test();
  return 0;
}
