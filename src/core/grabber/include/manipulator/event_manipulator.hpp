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
    modifier_flag_manager_.unlock();

    pointing_button_manager_.reset();

    // Do not call terminate_virtual_hid_keyboard
    virtual_hid_device_client_.terminate_virtual_hid_pointing();
  }

  void reset_modifier_flag_state(void) {
    modifier_flag_manager_.reset();
    // Do not call modifier_flag_manager_.unlock() here.
  }

  void reset_pointing_button_state(void) {
    auto bits = pointing_button_manager_.get_hid_report_bits();
    pointing_button_manager_.reset();
    if (bits) {
      virtual_hid_device_client_.reset_virtual_hid_pointing();
    }
  }

  void set_system_preferences_values(const system_preferences::values& values) {
    std::lock_guard<std::mutex> guard(system_preferences_values_mutex_);

    system_preferences_values_ = values;
  }

  void set_profile(const core_configuration::profile& profile) {
    simple_modifications_key_code_map_ = profile.get_simple_modifications_key_code_map(logger::get_logger());
    fn_function_keys_key_code_map_ = profile.get_fn_function_keys_key_code_map(logger::get_logger());
    standalone_keys_key_code_map_ = profile.get_standalone_keys_key_code_map(logger::get_logger());
    one_to_many_mappings_key_code_map_ = profile.get_one_to_many_mappings_key_code_map_(logger::get_logger());

    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    if (auto k = types::get_keyboard_type(profile.get_virtual_hid_keyboard().get_keyboard_type())) {
      properties.keyboard_type = *k;
    }
    properties.caps_lock_delay_milliseconds = pqrs::karabiner_virtual_hid_device::milliseconds(profile.get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds());
    virtual_hid_device_client_.initialize_virtual_hid_keyboard(properties);
    standalone_keys_delay_milliseconds_ = profile.get_virtual_hid_keyboard().get_standalone_keys_delay_milliseconds();
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
    modifier_flag_manager_.manipulate(modifier_flag::caps_lock,
                                      state ? modifier_flag_manager::operation::lock : modifier_flag_manager::operation::unlock);
  }

  void handle_keyboard_event(device_registry_entry_id device_registry_entry_id,
                             uint64_t timestamp,
                             key_code from_key_code,
                             bool pressed) {
    if (process_standalone_key(device_registry_entry_id, timestamp, from_key_code, pressed)) {
      return;
    }
    _handle_keyboard_event(device_registry_entry_id, timestamp, from_key_code, pressed);
  }

  void _handle_keyboard_event(device_registry_entry_id device_registry_entry_id,
                              uint64_t timestamp,
                              krbn::key_code from_key_code,
                              bool pressed) {
    if (process_one_to_many_mappings(from_key_code, pressed, timestamp)) {
        return;
    }
    key_code to_key_code = from_key_code;

    // ----------------------------------------
    // modify keys
    if (!pressed) {
      if (auto key_code = manipulated_keys_.find(device_registry_entry_id, from_key_code)) {
        manipulated_keys_.remove(device_registry_entry_id, from_key_code);
        to_key_code = *key_code;
      }
    } else {
      auto it = simple_modifications_key_code_map_.find(from_key_code);
      if (it != simple_modifications_key_code_map_.end()) {
        to_key_code = it->second;
        manipulated_keys_.add(device_registry_entry_id, from_key_code, to_key_code);
      }
    }

    // ----------------------------------------
    // modify fn+arrow, function keys
    if (!pressed) {
      if (auto key_code = manipulated_fn_keys_.find(device_registry_entry_id, to_key_code)) {
        manipulated_fn_keys_.remove(device_registry_entry_id, to_key_code);
        to_key_code = *key_code;
      }
    } else {
      boost::optional<key_code> key_code;

      if (modifier_flag_manager_.pressed(modifier_flag::fn)) {
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

          bool fn_pressed = modifier_flag_manager_.pressed(modifier_flag::fn);

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
        manipulated_fn_keys_.add(device_registry_entry_id, to_key_code, *key_code);
        to_key_code = *key_code;
      }
    }

    // ----------------------------------------

    post_key(to_key_code, pressed, timestamp);
  }

  void handle_pointing_event(device_registry_entry_id device_registry_entry_id,
                             uint64_t timestamp,
                             pointing_event pointing_event,
                             boost::optional<pointing_button> pointing_button,
                             CFIndex integer_value) {
    pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;

    switch (pointing_event) {
    case pointing_event::button:
      if (pointing_button && *pointing_button != pointing_button::zero) {
        pointing_button_manager_.manipulate(*pointing_button,
                                            integer_value ? pointing_button_manager::operation::increase : pointing_button_manager::operation::decrease);
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

    void add(device_registry_entry_id device_registry_entry_id,
             key_code from_key_code,
             key_code to_key_code) {
      std::lock_guard<std::mutex> guard(mutex_);

      manipulated_keys_.push_back(manipulated_key(device_registry_entry_id, from_key_code, to_key_code));
    }

    boost::optional<key_code> find(device_registry_entry_id device_registry_entry_id,
                                   key_code from_key_code) {
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
                key_code from_key_code) {
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
                      key_code from_key_code,
                      key_code to_key_code) : device_registry_entry_id_(device_registry_entry_id),
                                              from_key_code_(from_key_code),
                                              to_key_code_(to_key_code) {
      }

      device_registry_entry_id get_device_registry_entry_id(void) const { return device_registry_entry_id_; }
      key_code get_from_key_code(void) const { return from_key_code_; }
      key_code get_to_key_code(void) const { return to_key_code_; }

    private:
      device_registry_entry_id device_registry_entry_id_;
      key_code from_key_code_;
      key_code to_key_code_;
    };

    std::list<manipulated_key> manipulated_keys_;
    std::mutex mutex_;
  };

  bool process_one_to_many_mappings(krbn::key_code key_code, bool pressed, uint64_t timestamp) {
    auto it = one_to_many_mappings_key_code_map_.find(key_code);
    if (it != one_to_many_mappings_key_code_map_.end()) {
      auto to_key_codes = it->second;
      for (auto it2 = to_key_codes.begin(); it2 != to_key_codes.end(); it2++) {
        if ((krbn::types::get_modifier_flag(*it2) != krbn::modifier_flag::zero) == pressed) {
          post_key(*it2, pressed, timestamp);
        }
      }
      for (auto it2 = to_key_codes.begin(); it2 != to_key_codes.end(); it2++) {
        if ((krbn::types::get_modifier_flag(*it2) != krbn::modifier_flag::zero) != pressed) {
          post_key(*it2, pressed, timestamp);
        }
      }
      return true;
    }
    return false;
  }

  bool process_standalone_key(device_registry_entry_id device_registry_entry_id,
                              uint64_t timestamp,
                              krbn::key_code from_key_code,
                              bool pressed) {
    if (pressed) {
      if (standalone_keys_timer_ != nullptr) { // press the previous standalone key as the normal key
        standalone_keys_timer_ = nullptr;
        _handle_keyboard_event(device_registry_entry_id, timestamp, standalone_from_key_, pressed);
        handle_keyboard_event(device_registry_entry_id, timestamp, from_key_code, pressed);
        return true;
      } else if (standalone_keys_key_code_map_.find(from_key_code) != standalone_keys_key_code_map_.end()) { // from_key_code in standalone keys
        standalone_from_key_ = from_key_code;
        standalone_keys_timer_ = std::make_unique<gcd_utility::main_queue_timer>(
            dispatch_time(DISPATCH_TIME_NOW, standalone_keys_delay_milliseconds_ * NSEC_PER_MSEC),
            standalone_keys_delay_milliseconds_ * NSEC_PER_MSEC,
            0,
            ^{
              standalone_keys_timer_ = nullptr;
              _handle_keyboard_event(device_registry_entry_id, timestamp, from_key_code, pressed);
            });
        return true;
      } else {
        return false;
      }
    } else {
      if (from_key_code == standalone_from_key_ && standalone_keys_timer_ != nullptr) {
        standalone_keys_timer_ = nullptr;
        auto to_standalone_key_code = standalone_keys_key_code_map_.find(from_key_code)->second;
        post_key(to_standalone_key_code, true, timestamp);
        post_key(to_standalone_key_code, false, timestamp);
        return true;
      } else {
        return false;
      }
    }
  }

  bool post_standalone_key(krbn::key_code key_code, uint64_t timestamp) {
    auto it = standalone_keys_key_code_map_.find(key_code);
    if (it != standalone_keys_key_code_map_.end()) {
      post_key(it->second, true, timestamp);
      post_key(it->second, false, timestamp);
      return true;
    }
    return false;
  }

  void post_key(key_code key_code, bool pressed, uint64_t timestamp) {
    add_delay_to_continuous_event(timestamp);

    // pre-process modifier keys
    auto operation = pressed ? manipulator::modifier_flag_manager::operation::increase : manipulator::modifier_flag_manager::operation::decrease;
    auto modifier_flag = krbn::types::get_modifier_flag(key_code);
    if (modifier_flag != krbn::modifier_flag::zero) {
      modifier_flag_manager_.manipulate(modifier_flag, operation);
    }

    // send key event
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
  std::unordered_map<key_code, key_code> standalone_keys_key_code_map_;
  std::unordered_map<key_code, std::vector<key_code>> one_to_many_mappings_key_code_map_;
  key_code standalone_from_key_;
  std::unique_ptr<gcd_utility::main_queue_timer> standalone_keys_timer_;

  manipulated_keys manipulated_keys_;
  manipulated_keys manipulated_fn_keys_;

  uint64_t last_timestamp_;
  uint32_t standalone_keys_delay_milliseconds_;
};
}
}
