#pragma once

#include "hid_system_client.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
#include "userspace_types.hpp"
#include <IOKit/hidsystem/ev_keymap.h>
#include <thread>
#include <unordered_map>

class keyboard_event_output_manager final {
public:
  keyboard_event_output_manager(void) : hid_system_client_(logger::get_logger()),
                                        key_repeat_timer_(0) {}

  ~keyboard_event_output_manager(void) {
    stop_key_repeat();
  }

  void clear_fn_function_keys(void) {
    std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

    fn_function_keys_.clear();
  }

  void add_fn_function_key(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

    fn_function_keys_[from_key_code] = to_key_code;
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

    // ----------------------------------------
    // Change vk_f1 - vk_f12 and vk_fn_f1 - vk_fn_f12 to actual key code.
    // (eg. key_code::f1 or key_code::vk_consumer_brightness_down)

    bool is_standard_function_key = false;
    if (system_preferences::get_keyboard_fn_state()) {
      // "Use all F1, F2, etc. keys as standard function keys."
      is_standard_function_key = (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12);
    } else {
      is_standard_function_key = (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12);
    }

    krbn::key_code standard_function_key_code = krbn::key_code::vk_none;
    if (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12) {
      standard_function_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::f1) +
                                                  static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_f1));
    } else if (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12) {
      standard_function_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::f1) +
                                                  static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_fn_f1));
    }

    if (is_standard_function_key) {
      key_code = standard_function_key_code;
    } else {
      std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

      auto it = fn_function_keys_.find(standard_function_key_code);
      if (it == fn_function_keys_.end()) {
        key_code = standard_function_key_code;
      } else {
        key_code = it->second;
      }
    }

    // ----------------------------------------
    uint8_t new_key_code = 0;
    auto post_key_type = hid_system_client::post_key_type::key;
    switch (key_code) {
    case krbn::key_code::f1:
      new_key_code = 0x7a;
      break;
    case krbn::key_code::f2:
      new_key_code = 0x78;
      break;
    case krbn::key_code::f3:
      new_key_code = 0x63;
      break;
    case krbn::key_code::f4:
      new_key_code = 0x76;
      break;
    case krbn::key_code::f5:
      new_key_code = 0x60;
      break;
    case krbn::key_code::f6:
      new_key_code = 0x61;
      break;
    case krbn::key_code::f7:
      new_key_code = 0x62;
      break;
    case krbn::key_code::f8:
      new_key_code = 0x64;
      break;
    case krbn::key_code::f9:
      new_key_code = 0x65;
      break;
    case krbn::key_code::f10:
      new_key_code = 0x6d;
      break;
    case krbn::key_code::f11:
      new_key_code = 0x67;
      break;
    case krbn::key_code::f12:
      new_key_code = 0x6f;
      break;
    case krbn::key_code::vk_dashboard:
      new_key_code = 0x82;
      break;
    case krbn::key_code::vk_launchpad:
      new_key_code = 0x83;
      break;
    case krbn::key_code::vk_mission_control:
      new_key_code = 0xa0;
      break;

    case krbn::key_code::vk_consumer_brightness_down:
      new_key_code = NX_KEYTYPE_BRIGHTNESS_DOWN;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_brightness_up:
      new_key_code = NX_KEYTYPE_BRIGHTNESS_UP;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_illumination_down:
      new_key_code = NX_KEYTYPE_ILLUMINATION_DOWN;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_illumination_up:
      new_key_code = NX_KEYTYPE_ILLUMINATION_UP;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_previous:
      new_key_code = NX_KEYTYPE_PREVIOUS;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_play:
      new_key_code = NX_KEYTYPE_PLAY;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_next:
      new_key_code = NX_KEYTYPE_NEXT;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_mute:
      new_key_code = NX_KEYTYPE_MUTE;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_sound_down:
      new_key_code = NX_KEYTYPE_SOUND_DOWN;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    case krbn::key_code::vk_consumer_sound_up:
      new_key_code = NX_KEYTYPE_SOUND_UP;
      post_key_type = hid_system_client::post_key_type::aux_control_button;
      break;
    default:
      logger::get_logger().error("unknown key_code: 0x{1} @ {0}", __PRETTY_FUNCTION__, static_cast<uint32_t>(key_code));
      return;
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
  std::unordered_map<krbn::key_code, krbn::key_code> fn_function_keys_;
  std::mutex fn_function_keys_mutex_;
};
