#pragma once

#include "io_hid_post_event_wrapper.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
#include "userspace_defs.h"

class keyboard_event_output_manager final {
public:
  keyboard_event_output_manager(void) : key_repeat_timer_(0) {}

  ~keyboard_event_output_manager(void) {
    stop_key_repeat();
  }

  void stop_key_repeat(void) {
    if (key_repeat_timer_) {
      dispatch_source_cancel(key_repeat_timer_);
      dispatch_release(key_repeat_timer_);
      key_repeat_timer_ = 0;
    }
  }

  void post_modifier_flags(IOOptionBits flags) {
    stop_key_repeat();
    io_hid_post_event_wrapper_.post_modifier_flags(flags);
  }

  void post_key(uint8_t key_code, krbn_event_type event_type, IOOptionBits flags) {
    stop_key_repeat();

    bool standard_function_key = false;
    if (system_preferences::get_keyboard_fn_state()) {
      // "Use all F1, F2, etc. keys as standard function keys."
      standard_function_key = (KRBN_KEY_CODE_F1 <= key_code && key_code <= KRBN_KEY_CODE_F12);
    } else {
      standard_function_key = (KRBN_KEY_CODE_FN_F1 <= key_code && key_code <= KRBN_KEY_CODE_FN_F12);
    }

    uint8_t new_key_code = 0;
    auto post_key_type = io_hid_post_event_wrapper::post_key_type::key;
    switch (key_code) {
    case KRBN_KEY_CODE_F1:
    case KRBN_KEY_CODE_FN_F1:
      if (standard_function_key) {
        new_key_code = 0x7a;
      } else {
        new_key_code = NX_KEYTYPE_BRIGHTNESS_DOWN;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F2:
    case KRBN_KEY_CODE_FN_F2:
      if (standard_function_key) {
        new_key_code = 0x78;
      } else {
        new_key_code = NX_KEYTYPE_BRIGHTNESS_UP;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F3:
    case KRBN_KEY_CODE_FN_F3:
      if (standard_function_key) {
        new_key_code = 0x63;
      } else {
        // mission control
        new_key_code = 0xa0;
      }
      break;
    case KRBN_KEY_CODE_F4:
    case KRBN_KEY_CODE_FN_F4:
      if (standard_function_key) {
        new_key_code = 0x76;
      } else {
        // launchpad
        new_key_code = 0x83;
      }
      break;
    case KRBN_KEY_CODE_F5:
    case KRBN_KEY_CODE_FN_F5:
      if (standard_function_key) {
        new_key_code = 0x60;
      } else {
        new_key_code = NX_KEYTYPE_ILLUMINATION_DOWN;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F6:
    case KRBN_KEY_CODE_FN_F6:
      if (standard_function_key) {
        new_key_code = 0x61;
      } else {
        new_key_code = NX_KEYTYPE_ILLUMINATION_UP;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F7:
    case KRBN_KEY_CODE_FN_F7:
      if (standard_function_key) {
        new_key_code = 0x62;
      } else {
        new_key_code = NX_KEYTYPE_PREVIOUS;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F8:
    case KRBN_KEY_CODE_FN_F8:
      if (standard_function_key) {
        new_key_code = 0x64;
      } else {
        new_key_code = NX_KEYTYPE_PLAY;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F9:
    case KRBN_KEY_CODE_FN_F9:
      if (standard_function_key) {
        new_key_code = 0x65;
      } else {
        new_key_code = NX_KEYTYPE_NEXT;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F10:
    case KRBN_KEY_CODE_FN_F10:
      if (standard_function_key) {
        new_key_code = 0x6d;
      } else {
        new_key_code = NX_KEYTYPE_MUTE;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F11:
    case KRBN_KEY_CODE_FN_F11:
      if (standard_function_key) {
        new_key_code = 0x67;
      } else {
        new_key_code = NX_KEYTYPE_SOUND_DOWN;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    case KRBN_KEY_CODE_F12:
    case KRBN_KEY_CODE_FN_F12:
      if (standard_function_key) {
        new_key_code = 0x6f;
      } else {
        new_key_code = NX_KEYTYPE_SOUND_UP;
        post_key_type = io_hid_post_event_wrapper::post_key_type::aux_control_button;
      }
      break;
    }

    auto initial_key_repeat_milliseconds = system_preferences::get_initial_key_repeat_milliseconds();
    auto key_repeat_milliseconds = system_preferences::get_key_repeat_milliseconds();

    io_hid_post_event_wrapper_.post_key(post_key_type, new_key_code, event_type, flags, false);

    // set key repeat
    if (event_type == KRBN_EVENT_TYPE_KEY_DOWN) {
      bool repeat_target = true;
      if (post_key_type == io_hid_post_event_wrapper::post_key_type::aux_control_button) {
        if (new_key_code == NX_KEYTYPE_PLAY ||
            new_key_code == NX_KEYTYPE_MUTE) {
          repeat_target = false;
        }
      }

      if (repeat_target) {
        key_repeat_timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        if (!key_repeat_timer_) {
          logger::get_logger().error("failed to dispatch_source_create");
        } else {
          dispatch_source_set_timer(key_repeat_timer_,
                                    dispatch_time(DISPATCH_TIME_NOW, initial_key_repeat_milliseconds * NSEC_PER_MSEC),
                                    key_repeat_milliseconds * NSEC_PER_MSEC,
                                    0);
          dispatch_source_set_event_handler(key_repeat_timer_, ^{
            io_hid_post_event_wrapper_.post_key(post_key_type, new_key_code, event_type, flags, true);
          });
          dispatch_resume(key_repeat_timer_);
        }
      }
    }
  }

private:
  io_hid_post_event_wrapper io_hid_post_event_wrapper_;
  dispatch_source_t key_repeat_timer_;
};
