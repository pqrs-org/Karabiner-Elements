#pragma once

#include "hid_system_client.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
#include "userspace_types.hpp"
#include <IOKit/hidsystem/ev_keymap.h>

class keyboard_event_output_manager final {
public:
  keyboard_event_output_manager(void) : hid_system_client_(logger::get_logger()),
                                        key_repeat_timer_(0) {}

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
    hid_system_client_.post_modifier_flags(flags);
  }

  void post_key(krbn::key_code key_code, krbn::event_type event_type, IOOptionBits flags) {
    stop_key_repeat();

    bool standard_function_key = false;
    if (system_preferences::get_keyboard_fn_state()) {
      // "Use all F1, F2, etc. keys as standard function keys."
      standard_function_key = (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12);
    } else {
      standard_function_key = (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12);
    }

    uint8_t new_key_code = 0;
    auto post_key_type = hid_system_client::post_key_type::key;
    switch (key_code) {
    case krbn::key_code::vk_f1:
    case krbn::key_code::vk_fn_f1:
      if (standard_function_key) {
        new_key_code = 0x7a;
      } else {
        new_key_code = NX_KEYTYPE_BRIGHTNESS_DOWN;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f2:
    case krbn::key_code::vk_fn_f2:
      if (standard_function_key) {
        new_key_code = 0x78;
      } else {
        new_key_code = NX_KEYTYPE_BRIGHTNESS_UP;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f3:
    case krbn::key_code::vk_fn_f3:
      if (standard_function_key) {
        new_key_code = 0x63;
      } else {
        // mission control
        new_key_code = 0xa0;
      }
      break;
    case krbn::key_code::vk_f4:
    case krbn::key_code::vk_fn_f4:
      if (standard_function_key) {
        new_key_code = 0x76;
      } else {
        // launchpad
        new_key_code = 0x83;
      }
      break;
    case krbn::key_code::vk_f5:
    case krbn::key_code::vk_fn_f5:
      if (standard_function_key) {
        new_key_code = 0x60;
      } else {
        new_key_code = NX_KEYTYPE_ILLUMINATION_DOWN;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f6:
    case krbn::key_code::vk_fn_f6:
      if (standard_function_key) {
        new_key_code = 0x61;
      } else {
        new_key_code = NX_KEYTYPE_ILLUMINATION_UP;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f7:
    case krbn::key_code::vk_fn_f7:
      if (standard_function_key) {
        new_key_code = 0x62;
      } else {
        new_key_code = NX_KEYTYPE_PREVIOUS;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f8:
    case krbn::key_code::vk_fn_f8:
      if (standard_function_key) {
        new_key_code = 0x64;
      } else {
        new_key_code = NX_KEYTYPE_PLAY;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f9:
    case krbn::key_code::vk_fn_f9:
      if (standard_function_key) {
        new_key_code = 0x65;
      } else {
        new_key_code = NX_KEYTYPE_NEXT;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f10:
    case krbn::key_code::vk_fn_f10:
      if (standard_function_key) {
        new_key_code = 0x6d;
      } else {
        new_key_code = NX_KEYTYPE_MUTE;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f11:
    case krbn::key_code::vk_fn_f11:
      if (standard_function_key) {
        new_key_code = 0x67;
      } else {
        new_key_code = NX_KEYTYPE_SOUND_DOWN;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    case krbn::key_code::vk_f12:
    case krbn::key_code::vk_fn_f12:
      if (standard_function_key) {
        new_key_code = 0x6f;
      } else {
        new_key_code = NX_KEYTYPE_SOUND_UP;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
      break;
    default:
      new_key_code = static_cast<uint8_t>(key_code);
      break;
    }

    auto initial_key_repeat_milliseconds = system_preferences::get_initial_key_repeat_milliseconds();
    auto key_repeat_milliseconds = system_preferences::get_key_repeat_milliseconds();

    hid_system_client_.post_key(post_key_type, new_key_code, event_type, flags, false);

    // set key repeat
    if (event_type == krbn::event_type::key_down) {
      bool repeat_target = true;
      if (post_key_type == hid_system_client::post_key_type::aux_control_button) {
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
            hid_system_client_.post_key(post_key_type, new_key_code, event_type, flags, true);
          });
          dispatch_resume(key_repeat_timer_);
        }
      }
    }
  }

private:
  hid_system_client hid_system_client_;
  dispatch_source_t key_repeat_timer_;
};
