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

  event_manipulator(void) : event_dispatcher_manager_(),
                            key_repeat_manager_(event_dispatcher_manager_) {
  }

  bool is_ready(void) {
    return event_dispatcher_manager_.is_connected();
  }

  void reset(void) {
    key_repeat_manager_.stop();
    manipulated_keys_.clear();
    manipulated_fn_keys_.clear();
    modifier_flag_manager_.reset();
    modifier_flag_manager_.unlock();
    event_dispatcher_manager_.set_caps_lock_state(false);
  }

  void reset_modifier_flag_state(void) {
    modifier_flag_manager_.reset();
    // Do not call modifier_flag_manager_.unlock() here.
  }

  void set_system_preferences_values(const system_preferences::values& values) {
    std::lock_guard<std::mutex> guard(system_preferences_values_mutex_);

    system_preferences_values_ = values;
  }

  void clear_simple_modifications(void) {
    simple_modifications_.clear();
  }

  void add_simple_modification(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    simple_modifications_.add(from_key_code, to_key_code);
  }

  void clear_fn_function_keys(void) {
    fn_function_keys_.clear();
  }

  void add_fn_function_key(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    fn_function_keys_.add(from_key_code, to_key_code);
  }

  void create_event_dispatcher_client(void) {
    event_dispatcher_manager_.create_event_dispatcher_client();
  }

  void handle_keyboard_event(device_registry_entry_id device_registry_entry_id, krbn::key_code from_key_code, bool pressed) {
    krbn::key_code to_key_code = from_key_code;

    // ----------------------------------------
    // modify keys
    if (!pressed) {
      if (auto key_code = manipulated_keys_.find(device_registry_entry_id, from_key_code)) {
        to_key_code = *key_code;
        manipulated_keys_.remove(device_registry_entry_id, from_key_code);
      }
    } else {
      if (auto key_code = simple_modifications_.get(from_key_code)) {
        to_key_code = *key_code;
        manipulated_keys_.add(device_registry_entry_id, from_key_code, to_key_code);
      }
    }

    // ----------------------------------------
    // modify fn+arrow, function keys
    if (!pressed) {
      if (auto key_code = manipulated_fn_keys_.find(device_registry_entry_id, to_key_code)) {
        to_key_code = *key_code;
        manipulated_fn_keys_.remove(device_registry_entry_id, to_key_code);
      }
    } else {
      boost::optional<krbn::key_code> key_code;

      if (modifier_flag_manager_.pressed(krbn::modifier_flag::fn)) {
        switch (to_key_code) {
        case krbn::key_code::return_or_enter:
          key_code = krbn::key_code::keypad_enter;
          break;
        case krbn::key_code::delete_or_backspace:
          key_code = krbn::key_code::delete_forward;
          break;
        case krbn::key_code::right_arrow:
          key_code = krbn::key_code::end;
          break;
        case krbn::key_code::left_arrow:
          key_code = krbn::key_code::home;
          break;
        case krbn::key_code::down_arrow:
          key_code = krbn::key_code::page_down;
          break;
        case krbn::key_code::up_arrow:
          key_code = krbn::key_code::page_up;
          break;
        default:
          break;
        }
      }

      // f1-f12
      {
        auto key_code_value = static_cast<uint32_t>(to_key_code);
        if (kHIDUsage_KeyboardF1 <= key_code_value && key_code_value <= kHIDUsage_KeyboardF12) {
          bool keyboard_fn_state = false;
          {
            std::lock_guard<std::mutex> guard(system_preferences_values_mutex_);
            keyboard_fn_state = system_preferences_values_.get_keyboard_fn_state();
          }

          bool fn_pressed = modifier_flag_manager_.pressed(krbn::modifier_flag::fn);

          if ((fn_pressed && !keyboard_fn_state) ||
              (!fn_pressed && keyboard_fn_state)) {
            // change f1-f12 keys to media controls
            if (auto k = fn_function_keys_.get(to_key_code)) {
              key_code = *k;
            }
          }
        }
      }

      if (key_code) {
        manipulated_fn_keys_.add(device_registry_entry_id, to_key_code, *key_code);
        to_key_code = *key_code;
      }
    }

    // ----------------------------------------
    // Post input events to karabiner_event_dispatcher

    if (post_modifier_flag_event(to_key_code, pressed)) {
      key_repeat_manager_.stop();
      return;
    }

    if (to_key_code == krbn::key_code::caps_lock) {
      if (pressed) {
        toggle_caps_lock_state();
        key_repeat_manager_.stop();
      }
      return;
    }

    post_key(from_key_code, to_key_code, pressed);
  }

  void stop_key_repeat(void) {
    key_repeat_manager_.stop();
  }

  void refresh_caps_lock_led(void) {
    event_dispatcher_manager_.refresh_caps_lock_led();
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

  class simple_modifications final {
  public:
    void clear(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      map_.clear();
    }

    void add(krbn::key_code from_key_code, krbn::key_code to_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      map_[from_key_code] = to_key_code;
    }

    boost::optional<krbn::key_code> get(krbn::key_code from_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      auto it = map_.find(from_key_code);
      if (it != map_.end()) {
        return it->second;
      }

      return boost::none;
    }

  private:
    std::unordered_map<krbn::key_code, krbn::key_code> map_;
    std::mutex mutex_;
  };

  class key_repeat_manager final {
  public:
    key_repeat_manager(event_dispatcher_manager& event_dispatcher_manager) : event_dispatcher_manager_(event_dispatcher_manager),
                                                                             queue_(dispatch_queue_create(nullptr, nullptr)),
                                                                             timer_(dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, queue_)) {
    }

    ~key_repeat_manager(void) {
      if (timer_) {
        dispatch_source_cancel(timer_);
        dispatch_release(timer_);
        timer_ = 0;
      }

      dispatch_release(queue_);
    }

    void stop(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      if (from_key_code_) {
        from_key_code_ = boost::none;
        if (timer_) {
          dispatch_suspend(timer_);
        }
      }
    }

    void start(krbn::key_code from_key_code, krbn::key_code to_key_code, bool pressed, IOOptionBits flags,
               long initial_key_repeat_milliseconds, long key_repeat_milliseconds) {
      // stop key repeat before post key.
      if (pressed) {
        stop();
      } else {
        if (from_key_code_ && *from_key_code_ == from_key_code) {
          stop();
        }
      }

      // set key repeat
      if (pressed) {
        if (to_key_code == krbn::key_code::mute ||
            to_key_code == krbn::key_code::vk_consumer_play) {
          return;
        }

        if (timer_) {
          std::lock_guard<std::mutex> guard(mutex_);

          dispatch_source_set_timer(timer_,
                                    dispatch_time(DISPATCH_TIME_NOW, initial_key_repeat_milliseconds * NSEC_PER_MSEC),
                                    key_repeat_milliseconds * NSEC_PER_MSEC,
                                    0);
          dispatch_source_set_event_handler(timer_, ^{
            event_dispatcher_manager_.post_key(to_key_code, krbn::event_type::key_down, flags, true);
          });
          dispatch_resume(timer_);
          from_key_code_ = from_key_code;
        }
      }
    }

  private:
    event_dispatcher_manager& event_dispatcher_manager_;

    dispatch_queue_t queue_;
    dispatch_source_t timer_;

    boost::optional<krbn::key_code> from_key_code_;
    std::mutex mutex_;
  };

  bool post_modifier_flag_event(krbn::key_code key_code, bool pressed) {
    auto operation = pressed ? manipulator::modifier_flag_manager::operation::increase : manipulator::modifier_flag_manager::operation::decrease;

    auto modifier_flag = krbn::types::get_modifier_flag(key_code);
    if (modifier_flag != krbn::modifier_flag::zero) {
      modifier_flag_manager_.manipulate(modifier_flag, operation);

      event_dispatcher_manager_.post_modifier_flags(key_code, modifier_flag_manager_.get_io_option_bits());
      return true;
    }

    return false;
  }

  void toggle_caps_lock_state(void) {
    modifier_flag_manager_.manipulate(krbn::modifier_flag::caps_lock, modifier_flag_manager::operation::toggle_lock);
    if (modifier_flag_manager_.pressed(krbn::modifier_flag::caps_lock)) {
      event_dispatcher_manager_.set_caps_lock_state(true);
    } else {
      event_dispatcher_manager_.set_caps_lock_state(false);
    }
  }

  void post_key(krbn::key_code from_key_code, krbn::key_code to_key_code, bool pressed) {
    auto event_type = pressed ? krbn::event_type::key_down : krbn::event_type::key_up;
    auto flags = modifier_flag_manager_.get_io_option_bits();
    event_dispatcher_manager_.post_key(to_key_code, event_type, flags, false);

    long initial_key_repeat_milliseconds = 0;
    long key_repeat_milliseconds = 0;
    {
      std::lock_guard<std::mutex> guard(system_preferences_values_mutex_);
      initial_key_repeat_milliseconds = system_preferences_values_.get_initial_key_repeat_milliseconds();
      key_repeat_milliseconds = system_preferences_values_.get_key_repeat_milliseconds();
    }

    key_repeat_manager_.start(from_key_code, to_key_code, pressed, flags,
                              initial_key_repeat_milliseconds, key_repeat_milliseconds);
  }

  event_dispatcher_manager event_dispatcher_manager_;
  modifier_flag_manager modifier_flag_manager_;
  key_repeat_manager key_repeat_manager_;

  system_preferences::values system_preferences_values_;
  std::mutex system_preferences_values_mutex_;

  simple_modifications simple_modifications_;
  simple_modifications fn_function_keys_;

  manipulated_keys manipulated_keys_;
  manipulated_keys manipulated_fn_keys_;
};
}
