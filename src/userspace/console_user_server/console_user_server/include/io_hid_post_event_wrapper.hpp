#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>

#include "iokit_user_client.hpp"
#include "logger.hpp"
#include "userspace_defs.h"

class io_hid_post_event_wrapper final {
public:
  io_hid_post_event_wrapper(void) : iokit_user_client_(logger::get_logger(), kIOHIDSystemClass, kIOHIDParamConnectType) {}

  void post_modifier_flags(IOOptionBits flags) {
    NXEventData event;
    memset(&event, 0, sizeof(event));

    IOGPoint loc{0, 0};
    auto kr = iokit_user_client_.hid_post_event(NX_FLAGSCHANGED, loc, &event, kNXEventDataVersion, flags, kIOHIDSetGlobalEventFlags);
    if (KERN_SUCCESS != kr) {
      logger::get_logger().error("IOHIDPostEvent returned 0x{0:x}", kr);
    }
  }

  void post_key(krbn_key_code key_code, krbn_event_type event_type, IOOptionBits flags, bool repeat) {
    NXEventData event;
    memset(&event, 0, sizeof(event));
    event.key.origCharCode = 0;
    event.key.repeat = repeat;
    event.key.charSet = NX_ASCIISET;
    event.key.charCode = 0;
    event.key.keyCode = key_code;
    event.key.origCharSet = NX_ASCIISET;
    event.key.keyboardType = 0;

    IOGPoint loc{0, 0};
    auto kr = iokit_user_client_.hid_post_event(event_type == KRBN_EVENT_TYPE_KEY_DOWN ? NX_KEYDOWN : NX_KEYUP,
                                                loc,
                                                &event,
                                                kNXEventDataVersion,
                                                flags,
                                                0);

    if (KERN_SUCCESS != kr) {
      logger::get_logger().error("IOHIDPostEvent returned 0x{0:x}", kr);
    }
  }

  void post_aux_key(uint8_t key_code, krbn_event_type event_type, IOOptionBits flags, bool repeat) {
    NXEventData event;
    memset(&event, 0, sizeof(event));
    event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
    event.compound.misc.L[0] = (key_code << 16) | ((event_type == KRBN_EVENT_TYPE_KEY_DOWN ? NX_KEYDOWN : NX_KEYUP) << 8) | repeat;

    IOGPoint loc{0, 0};
    auto kr = iokit_user_client_.hid_post_event(NX_SYSDEFINED, loc, &event, kNXEventDataVersion, flags, 0);
    if (KERN_SUCCESS != kr) {
      logger::get_logger().error("IOHIDPostEvent returned 0x{0:x}", kr);
    }
  }

private:
  iokit_user_client iokit_user_client_;
};
