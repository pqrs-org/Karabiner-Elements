#pragma once

#include "boost_defs.hpp"

#include "hid_system_client.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
#include "types.hpp"
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/optional.hpp>
#include <thread>
#include <unordered_map>

class keyboard_event_output_manager final {
public:
  keyboard_event_output_manager(const keyboard_event_output_manager&) = delete;

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

  void post_key(krbn::key_code key_code, krbn::event_type event_type, IOOptionBits flags) {
    // ----------------------------------------
    // Change vk_f1 - vk_f12 and vk_fn_f1 - vk_fn_f12 to actual key code.
    // (eg. Change key_code::vk_f1 to key_code::f1 or key_code::vk_consumer_brightness_down)

    {
      bool is_standard_function_key = false;
      if (system_preferences::get_keyboard_fn_state()) {
        // "Use all F1, F2, etc. keys as standard function keys."
        is_standard_function_key = (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12);
      } else {
        is_standard_function_key = (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12);
      }

      boost::optional<krbn::key_code> standard_function_key_code;
      if (krbn::key_code::vk_f1 <= key_code && key_code <= krbn::key_code::vk_f12) {
        standard_function_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::f1) +
                                                    static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_f1));
      } else if (krbn::key_code::vk_fn_f1 <= key_code && key_code <= krbn::key_code::vk_fn_f12) {
        standard_function_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::f1) +
                                                    static_cast<uint32_t>(key_code) - static_cast<uint32_t>(krbn::key_code::vk_fn_f1));
      }
      // standard_function_key_code is in (krbn::key_code::f1 ... krbn::key_code::f12) or krbn::key_code::vk_none.

      if (standard_function_key_code) {
        if (is_standard_function_key) {
          key_code = *standard_function_key_code;
        } else {
          std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

          auto it = fn_function_keys_.find(*standard_function_key_code);
          if (it == fn_function_keys_.end()) {
            key_code = *standard_function_key_code;
          } else {
            key_code = it->second;
          }
        }
      }
    }

    // ----------------------------------------
    uint8_t new_key_code = 0;
    boost::optional<hid_system_client::post_key_type> post_key_type;

    {
      auto& map = krbn::types::get_mac_aux_control_button_map();
      auto it = map.find(key_code);
      if (it != map.end()) {
        new_key_code = it->second;
        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
    }
    {
      auto& map = krbn::types::get_mac_key_map();
      auto it = map.find(key_code);
      if (it != map.end()) {
        new_key_code = it->second;
        post_key_type = hid_system_client::post_key_type::key;
      }
    }

    if (!post_key_type) {
      logger::get_logger().warn("key_code:{1:#x} is unsupported key @ {0}", __PRETTY_FUNCTION__, static_cast<uint32_t>(key_code));
      return;
    }

    auto initial_key_repeat_milliseconds = system_preferences::get_initial_key_repeat_milliseconds();
    auto key_repeat_milliseconds = system_preferences::get_key_repeat_milliseconds();

    // stop key repeat before post key.
    if (event_type == krbn::event_type::key_down) {
      stop_key_repeat();
    } else {
      if (key_repeat_key_code_ && *key_repeat_key_code_ == key_code) {
        stop_key_repeat();
      }
    }

    // post key
    hid_system_client_.post_key(*post_key_type, new_key_code, event_type, flags, false);

    // set key repeat
    if (event_type == krbn::event_type::key_down) {
      bool repeat_target = true;
      if (*post_key_type == hid_system_client::post_key_type::aux_control_button) {
        if (new_key_code == NX_KEYTYPE_PLAY ||
            new_key_code == NX_KEYTYPE_MUTE) {
          repeat_target = false;
        }
      }

      if (repeat_target) {
        key_repeat_timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, dispatch_get_main_queue());
        if (!key_repeat_timer_) {
          logger::get_logger().error("failed to dispatch_source_create");
        } else {
          dispatch_source_set_timer(key_repeat_timer_,
                                    dispatch_time(DISPATCH_TIME_NOW, initial_key_repeat_milliseconds * NSEC_PER_MSEC),
                                    key_repeat_milliseconds * NSEC_PER_MSEC,
                                    0);
          dispatch_source_set_event_handler(key_repeat_timer_, ^{
            hid_system_client_.post_key(*post_key_type, new_key_code, event_type, flags, true);
          });
          dispatch_resume(key_repeat_timer_);
          key_repeat_key_code_ = key_code;
        }
      }
    }
  }

private:
  hid_system_client hid_system_client_;
  dispatch_source_t key_repeat_timer_;
  boost::optional<krbn::key_code> key_repeat_key_code_;
  std::unordered_map<krbn::key_code, krbn::key_code> fn_function_keys_;
  std::mutex fn_function_keys_mutex_;
};
