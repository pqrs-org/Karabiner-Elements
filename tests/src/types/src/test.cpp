#include "device_identifiers_test.hpp"
#include "errors_test.hpp"
#include "event_origin_test.hpp"
#include "grabbable_state_test.hpp"
#include "modifier_flag_test.hpp"
#include "momentary_switch_event_test.hpp"
#include "mouse_key_test.hpp"
#include "notification_message_test.hpp"
#include "operation_type_test.hpp"
#include "pointing_motion_test.hpp"
#include "software_function_test.hpp"
#include "virtual_hid_devices_state_test.hpp"

int main(void) {
  run_device_identifiers_test();
  run_errors_test();
  run_event_origin_test();
  run_grabbable_state_test();
  run_modifier_flag_test();
  run_momentary_switch_event_test();
  run_mouse_key_test();
  run_notification_message_test();
  run_operation_type_test();
  run_pointing_motion_test();
  run_software_function_test();
  run_virtual_hid_devices_state_test();
  return 0;
}
