#pragma once

#include "user_client.hpp"
#include "userspace_defs.h"

class io_hid_post_event_wrapper final {
public:
  void start(void) {
    user_client_.open(kIOHIDSystemClass, kIOHIDParamConnectType);
  }

  void post_modifier_flags(IOOptionBits flags) {
    auto connect = user_client_.get_connect();
    if (!connect) {
      return;
    }

    NXEventData event;
    memset(&event, 0, sizeof(event));

    IOGPoint loc = {0, 0};
    auto kr = IOHIDPostEvent(connect, NX_FLAGSCHANGED, loc, &event, kNXEventDataVersion, flags, kIOHIDSetGlobalEventFlags);

    if (KERN_SUCCESS != kr) {
      std::cerr << "IOHIDPostEvent returned 0x" << std::hex << kr << std::dec << std::endl;
    }
  }

  void post_key(uint8_t key_code, enum krbn_ev_type ev_type, IOOptionBits flags, bool repeat) {
    auto connect = user_client_.get_connect();
    if (!connect) {
      return;
    }

    NXEventData event;
    memset(&event, 0, sizeof(event));
    event.key.origCharCode = 0;
    event.key.repeat = repeat;
    event.key.charSet = NX_ASCIISET;
    event.key.charCode = 0;
    event.key.keyCode = key_code;
    event.key.origCharSet = NX_ASCIISET;
    event.key.keyboardType = 0;

    IOGPoint loc = {0, 0};
    auto kr = IOHIDPostEvent(connect,
                             ev_type == KRBN_EV_TYPE_KEY_DOWN ? NX_KEYDOWN : NX_KEYUP,
                             loc,
                             &event,
                             kNXEventDataVersion,
                             flags,
                             0);

    if (KERN_SUCCESS != kr) {
      std::cerr << "IOHIDPostEvent returned 0x" << std::hex << kr << std::dec << std::endl;
    }
  }

  void post_aux_key(uint8_t key_code, bool key_down, IOOptionBits flags) {
    auto connect = user_client_.get_connect();
    if (!connect) {
      return;
    }

    NXEventData event;
    memset(&event, 0, sizeof(event));
    event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
    event.compound.misc.L[0] = (key_code << 16) | ((key_down ? NX_KEYDOWN : NX_KEYUP) << 8);

    IOGPoint loc = {0, 0};
    kern_return_t kr = IOHIDPostEvent(connect, NX_SYSDEFINED, loc, &event, kNXEventDataVersion, flags, 0);
    if (KERN_SUCCESS != kr) {
      std::cerr << "IOHIDPostEvent returned 0x" << std::hex << kr << std::dec << std::endl;
    }
  }

private:
  user_client user_client_;
};
