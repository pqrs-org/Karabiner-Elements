#include "errors_test.hpp"
#include "event_definition_test.hpp"
#include "from_modifiers_definition_test.hpp"
#include "modifier_definition_test.hpp"

int main(void) {
  run_errors_test();
  run_event_definition_test();
  run_from_modifiers_definition_test();
  run_modifier_definition_test();
  return 0;
}
