#include "errors_test.hpp"
#include "manipulator_basic_test.hpp"
#include "simultaneous_options_test.hpp"
#include "to_after_key_up_test.hpp"
#include "to_delayed_action_test.hpp"
#include "to_if_alone_test.hpp"
#include "to_if_held_down_test.hpp"

int main(void) {
  run_errors_test();
  run_manipulator_basic_test();
  run_simultaneous_options_test();
  run_to_after_key_up_test();
  run_to_delayed_action_test();
  run_to_if_alone_test();
  run_to_if_held_down_test();
  return 0;
}
