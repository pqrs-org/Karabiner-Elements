#pragma once

#include "boost_defs.hpp"

#include "event_dispatcher_manager.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "modifier_flag_manager.hpp"
#include "system_preferences.hpp"
#include "types.hpp"
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/optional.hpp>
#include <list>
#include <thread>
#include <unordered_map>

namespace manipulator {
class event_manipulator final {
public:
  event_manipulator(const event_manipulator&) = delete;

  event_manipulator(void) : key_repeat_queue_(dispatch_queue_create(nullptr, nullptr)),
                            key_repeat_timer_(0) {
  }

  ~event_manipulator(void) {
    stop_key_repeat();
    dispatch_release(key_repeat_queue_);
  }

  bool is_ready(void) {
    return event_dispatcher_manager_.is_connected();
  }

  void reset(void) {
    stop_key_repeat();
    manipulated_keys_.clear();
    modifier_flag_manager_.reset();
    event_dispatcher_manager_.set_caps_lock_state(false);
  }

  void clear_simple_modifications(void) {
    std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

    simple_modifications_.clear();
  }

  void add_simple_modification(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

    simple_modifications_[from_key_code] = to_key_code;
  }

  void clear_fn_function_keys(void) {
    std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

    fn_function_keys_.clear();
  }

  void add_fn_function_key(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    std::lock_guard<std::mutex> guard(fn_function_keys_mutex_);

    fn_function_keys_[from_key_code] = to_key_code;
  }

  void create_event_dispatcher_client(void) {
    event_dispatcher_manager_.create_event_dispatcher_client();
  }

  void handle_keyboard_event(device_registry_entry_id device_registry_entry_id, krbn::key_code key_code, bool pressed) {
    // ----------------------------------------
    // modify usage
    if (!pressed) {
      if (auto to_key_code = manipulated_keys_.find(device_registry_entry_id, key_code)) {
        key_code = *to_key_code;
        manipulated_keys_.remove(device_registry_entry_id, key_code);
      }
    } else {
      std::lock_guard<std::mutex> guard(simple_modifications_mutex_);

      auto it = simple_modifications_.find(key_code);
      if (it != simple_modifications_.end()) {
        manipulated_keys_.add(device_registry_entry_id, key_code, it->second);
        key_code = it->second;
      }
    }

#if 0
    // modify fn+arrow, function keys
    if (!pressed) {
      auto it = device.get_fn_changed_keys().find(key_code);
      if (it != device.get_fn_changed_keys().end()) {
        key_code = it->second;
        device.get_fn_changed_keys().erase(it);
      }
    } else {
      auto k = static_cast<uint32_t>(key_code);
      auto new_key_code = key_code;
      if (modifier_flag_manager_.pressed(krbn::modifier_flag::fn)) {
        if (k == kHIDUsage_KeyboardReturnOrEnter) {
          new_key_code = krbn::key_code(kHIDUsage_KeypadEnter);
        } else if (k == kHIDUsage_KeyboardDeleteOrBackspace) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardDeleteForward);
        } else if (k == kHIDUsage_KeyboardRightArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardEnd);
        } else if (k == kHIDUsage_KeyboardLeftArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardHome);
        } else if (k == kHIDUsage_KeyboardDownArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardPageDown);
        } else if (k == kHIDUsage_KeyboardUpArrow) {
          new_key_code = krbn::key_code(kHIDUsage_KeyboardPageUp);
        } else if (kHIDUsage_KeyboardF1 <= k && k <= kHIDUsage_KeyboardF12) {
          new_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_fn_f1) + k - kHIDUsage_KeyboardF1);
        }
      } else {
        if (kHIDUsage_KeyboardF1 <= k && k <= kHIDUsage_KeyboardF12) {
          new_key_code = krbn::key_code(static_cast<uint32_t>(krbn::key_code::vk_f1) + k - kHIDUsage_KeyboardF1);
        }
      }
      if (key_code != new_key_code) {
        (device.get_fn_changed_keys())[key_code] = new_key_code;
        key_code = new_key_code;
      }
    }

    // ----------------------------------------
    // Post input events

    if (post_modifier_flag_event(key_code, pressed) ||
        toggle_caps_lock_state(key_code, pressed)) {
      console_user_client_.stop_key_repeat();
      return;
    }

    auto event_type = pressed ? krbn::event_type::key_down : krbn::event_type::key_up;
    console_user_client_.post_key(key_code, event_type, modifier_flag_manager_.get_io_option_bits());
#endif
  }

#if 0
  bool post_modifier_flag_event2(krbn::key_code key_code, bool pressed) {
    auto operation = pressed ? manipulator::modifier_flag_manager::operation::increase : manipulator::modifier_flag_manager::operation::decrease;

    auto modifier_flag = krbn::types::get_modifier_flag(key_code);
    if (modifier_flag != krbn::modifier_flag::zero) {
      modifier_flag_manager_.manipulate(modifier_flag, operation);

      // reset modifier_flags state if all keys are released.
      if (get_all_devices_pressed_keys_count() == 0) {
        modifier_flag_manager_.reset();
      }

      event_manipulator_.post_modifier_flags(key_code, modifier_flag_manager_.get_io_option_bits());
      return true;
    }

    return false;
  }

  bool post_caps_lock_key2(krbn::key_code key_code, bool pressed) {
    if (key_code != krbn::key_code(kHIDUsage_KeyboardCapsLock)) {
      return false;
    }

    if (pressed) {
      event_manipulator_.post_caps_lock_key();
    }
    return true;
  }
#endif

  void stop_key_repeat(void) {
    if (key_repeat_timer_) {
      dispatch_source_cancel(key_repeat_timer_);
      dispatch_release(key_repeat_timer_);
      key_repeat_timer_ = 0;
    }
  }

  void post_modifier_flags(krbn::key_code key_code, IOOptionBits flags) {
    stop_key_repeat();
    event_dispatcher_manager_.post_modifier_flags(key_code, flags);
  }

  void toggle_caps_lock_state(void) {
    stop_key_repeat();
    event_dispatcher_manager_.toggle_caps_lock_state();
  }

  void refresh_caps_lock_led(void) {
    event_dispatcher_manager_.refresh_caps_lock_led();
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
    //    uint8_t new_key_code = 0;
    //    boost::optional<hid_system_client::post_key_type> post_key_type;

    {
      auto& map = krbn::types::get_mac_aux_control_button_map();
      auto it = map.find(key_code);
      if (it != map.end()) {
        //        new_key_code = it->second;
        //        post_key_type = hid_system_client::post_key_type::aux_control_button;
      }
    }
    {
      auto& map = krbn::types::get_mac_key_map();
      auto it = map.find(key_code);
      if (it != map.end()) {
        //        new_key_code = it->second;
        //        post_key_type = hid_system_client::post_key_type::key;
      }
    }

    //    if (!post_key_type) {
    //      logger::get_logger().warn("key_code:{1:#x} is unsupported key @ {0}", __PRETTY_FUNCTION__, static_cast<uint32_t>(key_code));
    //      return;
    //    }

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
    //    hid_system_client_.post_key(*post_key_type, new_key_code, event_type, flags, false);

    // set key repeat
    if (event_type == krbn::event_type::key_down) {
      bool repeat_target = true;
      if (key_code == krbn::key_code::mute ||
          key_code == krbn::key_code::vk_consumer_play) {
        repeat_target = false;
      }

      if (repeat_target) {
        key_repeat_timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, key_repeat_queue_);
        if (!key_repeat_timer_) {
          logger::get_logger().error("failed to dispatch_source_create");
        } else {
          dispatch_source_set_timer(key_repeat_timer_,
                                    dispatch_time(DISPATCH_TIME_NOW, initial_key_repeat_milliseconds * NSEC_PER_MSEC),
                                    key_repeat_milliseconds * NSEC_PER_MSEC,
                                    0);
          dispatch_source_set_event_handler(key_repeat_timer_, ^{
                                                //            hid_system_client_.post_key(*post_key_type, new_key_code, event_type, flags, true);
                                            });
          dispatch_resume(key_repeat_timer_);
          key_repeat_key_code_ = key_code;
        }
      }
    }
  }

private:
  class manipulated_keys final {
  public:
    void clear(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.clear();
    }

    void add(device_registry_entry_id device_registry_entry_id,
             krbn::key_code from_key_code,
             krbn::key_code to_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.push_back(manipulated_key(device_registry_entry_id, from_key_code, to_key_code));
    }

    boost::optional<krbn::key_code> find(device_registry_entry_id device_registry_entry_id,
                                         krbn::key_code from_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      for (const auto& v : manipulated_keys_) {
        if (v.get_device_registry_entry_id() == device_registry_entry_id &&
            v.get_from_key_code() == from_key_code) {
          return v.get_to_key_code();
        }
      }
      return boost::none;
    }

    void remove(device_registry_entry_id device_registry_entry_id,
                krbn::key_code from_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.remove_if([&](const manipulated_key& v) {
        return v.get_device_registry_entry_id() == device_registry_entry_id &&
               v.get_from_key_code() == from_key_code;
      });
    }

  private:
    class manipulated_key final {
    public:
      manipulated_key(device_registry_entry_id device_registry_entry_id,
                      krbn::key_code from_key_code,
                      krbn::key_code to_key_code) : device_registry_entry_id_(device_registry_entry_id),
                                                    from_key_code_(from_key_code),
                                                    to_key_code_(to_key_code) {
      }

      device_registry_entry_id get_device_registry_entry_id(void) const { return device_registry_entry_id_; }
      krbn::key_code get_from_key_code(void) const { return from_key_code_; }
      krbn::key_code get_to_key_code(void) const { return to_key_code_; }

    private:
      device_registry_entry_id device_registry_entry_id_;
      krbn::key_code from_key_code_;
      krbn::key_code to_key_code_;
    };

    std::list<manipulated_key> manipulated_keys_;
    std::mutex mutex_;
  };

  event_dispatcher_manager event_dispatcher_manager_;
  modifier_flag_manager modifier_flag_manager_;

  dispatch_queue_t key_repeat_queue_;
  dispatch_source_t key_repeat_timer_;

  boost::optional<krbn::key_code> key_repeat_key_code_;

  std::unordered_map<krbn::key_code, krbn::key_code> simple_modifications_;
  std::mutex simple_modifications_mutex_;

  std::unordered_map<krbn::key_code, krbn::key_code> fn_function_keys_;
  std::mutex fn_function_keys_mutex_;

  manipulated_keys manipulated_keys_;
};
}
