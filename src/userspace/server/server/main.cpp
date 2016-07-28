#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <iostream>

#include "io_hid_post_event_wrapper.hpp"

int main(int argc, const char* argv[]) {
  io_hid_post_event_wrapper wrapper;
  wrapper.start();

  wrapper.post_modifier_flags(NX_SHIFTMASK);
  wrapper.post_modifier_flags(0);

  wrapper.post_aux_key(NX_KEYTYPE_MUTE, true);
  wrapper.post_aux_key(NX_KEYTYPE_MUTE, false);

  return 0;
}
