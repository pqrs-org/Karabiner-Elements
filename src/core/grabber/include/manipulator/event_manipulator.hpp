#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "modifier_flag_manager.hpp"
#include "pointing_button_manager.hpp"
#include "system_preferences.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/optional.hpp>
#include <list>
#include <thread>
#include <unordered_map>

namespace krbn {
namespace manipulator {
class event_manipulator final {
public:
  event_manipulator(const event_manipulator&) = delete;

  event_manipulator(virtual_hid_device_client& virtual_hid_device_client) : virtual_hid_device_client_(virtual_hid_device_client),
                                                                            last_timestamp_(0) {
  }

  ~event_manipulator(void) {
  }

  enum class ready_state {
    ready,
    virtual_hid_device_client_is_not_ready,
    virtual_hid_keyboard_is_not_ready,
  };

  ready_state is_ready(void) {
    if (!virtual_hid_device_client_.is_connected()) {
      return ready_state::virtual_hid_device_client_is_not_ready;
    }
    if (!virtual_hid_device_client_.is_virtual_hid_keyboard_initialized()) {
      return ready_state::virtual_hid_keyboard_is_not_ready;
    }
    return ready_state::ready;
  }

  void reset(void) {
    manipulated_keys_.clear();
    manipulated_fn_keys_.clear();

    modifier_flag_manager_.reset();

    pointing_button_manager_.reset();

    // Do not call terminate_virtual_hid_keyboard
    virtual_hid_device_client_.terminate_virtual_hid_pointing();
  }

  void erase_all_active_modifier_flags(device_id device_id, bool include_lock) {
    if (include_lock) {
      modifier_flag_manager_.erase_all_active_modifier_flags(device_id);
    } else {
      modifier_flag_manager_.erase_all_active_modifier_flags_except_lock(device_id);
    }

    // TODO
    // Post modifier flag event
  }

  void erase_all_active_pointing_buttons(device_id device_id, bool include_lock) {
    auto previous_bits = pointing_button_manager_.get_hid_report_bits();

    if (include_lock) {
      pointing_button_manager_.erase_all_active_pointing_buttons(device_id);
    } else {
      pointing_button_manager_.erase_all_active_pointing_buttons_except_lock(device_id);
    }

    auto bits = pointing_button_manager_.get_hid_report_bits();

    if (bits != previous_bits) {
      pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
      report.buttons[0] = (bits >> 0) & 0xff;
      report.buttons[1] = (bits >> 8) & 0xff;
      report.buttons[2] = (bits >> 16) & 0xff;
      report.buttons[3] = (bits >> 24) & 0xff;
      virtual_hid_device_client_.post_pointing_input_report(report);
    }
  }

  void set_system_preferences_values(const system_preferences::values& values) {
    std::lock_guard<std::mutex> guard(system_preferences_values_mutex_);

    system_preferences_values_ = values;
  }

  void set_profile(const core_configuration::profile& profile) {
    simple_modifications_key_code_map_ = profile.get_simple_modifications_key_code_map(logger::get_logger());
    fn_function_keys_key_code_map_ = profile.get_fn_function_keys_key_code_map(logger::get_logger());

    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    if (auto k = types::get_keyboard_type(profile.get_virtual_hid_keyboard().get_keyboard_type())) {
      properties.keyboard_type = *k;
    }
    properties.caps_lock_delay_milliseconds = pqrs::karabiner_virtual_hid_device::milliseconds(profile.get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds());
    virtual_hid_device_client_.initialize_virtual_hid_keyboard(properties);
  }

  void unset_profile(void) {
    simple_modifications_key_code_map_ = std::unordered_map<key_code, key_code>();
    fn_function_keys_key_code_map_ = std::unordered_map<key_code, key_code>();
  }

  void initialize_virtual_hid_pointing(void) {
    virtual_hid_device_client_.initialize_virtual_hid_pointing();
  }

  void terminate_virtual_hid_pointing(void) {
    virtual_hid_device_client_.terminate_virtual_hid_pointing();
  }

  void set_caps_lock_state(bool state) {
    modifier_flag_manager::active_modifier_flag active_modifier_flag(modifier_flag_manager::active_modifier_flag::type::increase_lock,
                                                                     modifier_flag::caps_lock,
                                                                     device_id(0));
    if (state) {
      modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
    } else {
      modifier_flag_manager_.erase_active_modifier_flag(active_modifier_flag);
    }
  }

  void handle_keyboard_event(device_id device_id,
                             uint64_t timestamp,
                             key_code from_key_code,
                             bool pressed) {
    key_code to_key_code = from_key_code;

    // ----------------------------------------
    // modify keys
    if (!pressed) {
      if (auto key_code = manipulated_keys_.find(device_id, from_key_code)) {
        manipulated_keys_.remove(device_id, from_key_code);
        to_key_code = *key_code;
      }
    } else {
      auto it = simple_modifications_key_code_map_.find(from_key_code);
      if (it != simple_modifications_key_code_map_.end()) {
        to_key_code = it->second;
        manipulated_keys_.add(device_id, from_key_code, to_key_code);
      }
    }

    // ----------------------------------------
    // modify fn+arrow, function keys
    if (!pressed) {
      if (auto key_code = manipulated_fn_keys_.find(device_id, to_key_code)) {
        manipulated_fn_keys_.remove(device_id, to_key_code);
        to_key_code = *key_code;
      }
    } else {
      boost::optional<key_code> key_code;

      if (modifier_flag_manager_.is_pressed(modifier_flag::fn)) {
        switch (to_key_code) {
        case key_code::return_or_enter:
          key_code = key_code::keypad_enter;
          break;
        case key_code::delete_or_backspace:
          key_code = key_code::delete_forward;
          break;
        case key_code::right_arrow:
          key_code = key_code::end;
          break;
        case key_code::left_arrow:
          key_code = key_code::home;
          break;
        case key_code::down_arrow:
          key_code = key_code::page_down;
          break;
        case key_code::up_arrow:
          key_code = key_code::page_up;
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

          bool fn_pressed = modifier_flag_manager_.is_pressed(modifier_flag::fn);

          if ((fn_pressed && keyboard_fn_state) ||
              (!fn_pressed && !keyboard_fn_state)) {
            // change f1-f12 keys to media controls
            auto it = fn_function_keys_key_code_map_.find(to_key_code);
            if (it != fn_function_keys_key_code_map_.end()) {
              key_code = it->second;
            }
          }
        }
      }

      if (key_code) {
        manipulated_fn_keys_.add(device_id, to_key_code, *key_code);
        to_key_code = *key_code;
      }
    }

    // ----------------------------------------
    if (post_modifier_flag_event(device_id, to_key_code, pressed, timestamp)) {
      return;
    }

    post_key(to_key_code, pressed, timestamp);
  }

  void handle_pointing_event(device_id device_id,
                             uint64_t timestamp,
                             pointing_event pointing_event,
                             boost::optional<pointing_button> pointing_button,
                             CFIndex integer_value) {
    pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;

    switch (pointing_event) {
    case pointing_event::button:
      if (pointing_button && *pointing_button != pointing_button::zero) {
        pointing_button_manager::active_pointing_button active_pointing_button(pointing_button_manager::active_pointing_button::type::increase,
                                                                               *pointing_button,
                                                                               device_id);
        if (integer_value) {
          pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
        } else {
          pointing_button_manager_.erase_active_pointing_button(active_pointing_button);
        }
      }
      break;

    case pointing_event::x:
      report.x = integer_value;
      break;

    case pointing_event::y:
      report.y = integer_value;
      break;

    case pointing_event::vertical_wheel:
      report.vertical_wheel = integer_value;
      break;

    case pointing_event::horizontal_wheel:
      report.horizontal_wheel = integer_value;
      break;

    default:
      break;
    }

    auto bits = pointing_button_manager_.get_hid_report_bits();
    report.buttons[0] = (bits >> 0) & 0xff;
    report.buttons[1] = (bits >> 8) & 0xff;
    report.buttons[2] = (bits >> 16) & 0xff;
    report.buttons[3] = (bits >> 24) & 0xff;
    virtual_hid_device_client_.post_pointing_input_report(report);
  }

  void stop_key_repeat(void) {
    virtual_hid_device_client_.reset_virtual_hid_keyboard();
  }

private:
  class manipulated_keys final {
  public:
    manipulated_keys(const manipulated_keys&) = delete;

    manipulated_keys(void) {
    }

    void clear(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.clear();
    }

    void add(device_id device_id,
             key_code from_key_code,
             key_code to_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.push_back(manipulated_key(device_id, from_key_code, to_key_code));
    }

    boost::optional<key_code> find(device_id device_id,
                                   key_code from_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      for (const auto& v : manipulated_keys_) {
        if (v.get_device_id() == device_id &&
            v.get_from_key_code() == from_key_code) {
          return v.get_to_key_code();
        }
      }
      return boost::none;
    }

    void remove(device_id device_id,
                key_code from_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.remove_if([&](const manipulated_key& v) {
        return v.get_device_id() == device_id &&
               v.get_from_key_code() == from_key_code;
      });
    }

  private:
    class manipulated_key final {
    public:
      manipulated_key(device_id device_id,
                      key_code from_key_code,
                      key_code to_key_code) : device_id_(device_id),
                                              from_key_code_(from_key_code),
                                              to_key_code_(to_key_code) {
      }

      device_id get_device_id(void) const { return device_id_; }
      key_code get_from_key_code(void) const { return from_key_code_; }
      key_code get_to_key_code(void) const { return to_key_code_; }

    private:
      device_id device_id_;
      key_code from_key_code_;
      key_code to_key_code_;
    };

    std::list<manipulated_key> manipulated_keys_;
    std::mutex mutex_;
  };

  bool post_modifier_flag_event(device_id device_id,
                                key_code key_code,
                                bool pressed,
                                uint64_t timestamp) {
    auto modifier_flag = types::get_modifier_flag(key_code);
    if (modifier_flag != modifier_flag::zero) {
      modifier_flag_manager::active_modifier_flag active_modifier_flag(modifier_flag_manager::active_modifier_flag::type::increase,
                                                                       modifier_flag,
                                                                       device_id);
      if (pressed) {
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      } else {
        modifier_flag_manager_.erase_active_modifier_flag(active_modifier_flag);
      }

      post_key(key_code, pressed, timestamp);
      return true;
    }

    return false;
  }

  void post_key(key_code key_code, bool pressed, uint64_t timestamp) {
    add_delay_to_continuous_event(timestamp);

    if (auto usage_page = types::get_usage_page(key_code)) {
      if (auto usage = types::get_usage(key_code)) {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
        keyboard_event.usage_page = *usage_page;
        keyboard_event.usage = *usage;
        keyboard_event.value = pressed;
        virtual_hid_device_client_.dispatch_keyboard_event(keyboard_event);
      }
    }
  }

  void add_delay_to_continuous_event(uint64_t timestamp) {
    if (timestamp != last_timestamp_) {
      last_timestamp_ = timestamp;

    } else {
      // We need to add a delay to continous events to ensure the key events order in WindowServer.
      //
      // Unless the delay, application will receive FlagsChanged event after KeyDown events even if the modifier key is sent before.
      //
      // Example of no delay:
      //   In event_manipulator:
      //     1. send shift key down
      //     2. send tab key down
      //     3. send tab key up
      //     4. send shift key up
      //
      //   In application
      //     1. KeyDown tab
      //     2. FlagsChanged shift
      //     3. KeyUp tab
      //     4. FlagsChanged shift
      //
      // We need the delay to avoid this order changes.

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  virtual_hid_device_client& virtual_hid_device_client_;
  modifier_flag_manager modifier_flag_manager_;
  pointing_button_manager pointing_button_manager_;

  system_preferences::values system_preferences_values_;
  std::mutex system_preferences_values_mutex_;

  std::unordered_map<key_code, key_code> simple_modifications_key_code_map_;
  std::unordered_map<key_code, key_code> fn_function_keys_key_code_map_;

  manipulated_keys manipulated_keys_;
  manipulated_keys manipulated_fn_keys_;

  uint64_t last_timestamp_;
};
} // namespace manipulator
} // namespace krbn
