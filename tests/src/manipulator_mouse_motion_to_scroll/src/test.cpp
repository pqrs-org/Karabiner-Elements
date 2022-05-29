#include "counter_test.hpp"
#include "errors_test.hpp"
#include "options_test.hpp"

int main(void) {
  run_counter_test();
  run_errors_test();
  run_options_test();
  return 0;
}
