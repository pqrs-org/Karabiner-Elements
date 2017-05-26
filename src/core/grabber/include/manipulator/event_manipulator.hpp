#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "logger.hpp"
#include "manipulator_manager.hpp"
#include "modifier_flag_manager.hpp"
#include "pointing_button_manager.hpp"
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

  event_manipulator(virtual_hid_device_client& virtual_hid_device_client) : virtual_hid_device_client_(virtual_hid_device_client) {
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
    if (!virtual_hid_device_client_.is_virtual_hid_keyboard_ready()) {
      return ready_state::virtual_hid_keyboard_is_not_ready;
    }
    return ready_state::ready;
  }

  void reset(void) {
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

  void set_profile(const core_configuration::profile& profile) {
    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    if (auto k = types::get_keyboard_type(profile.get_virtual_hid_keyboard().get_keyboard_type())) {
      properties.keyboard_type = *k;
    }
    properties.caps_lock_delay_milliseconds = pqrs::karabiner_virtual_hid_device::milliseconds(profile.get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds());
    virtual_hid_device_client_.initialize_virtual_hid_keyboard(properties);
  }

  void unset_profile(void) {
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

  void stop_key_repeat(void) {
    virtual_hid_device_client_.reset_virtual_hid_keyboard();
  }

private:
  virtual_hid_device_client& virtual_hid_device_client_;
  modifier_flag_manager modifier_flag_manager_;
  pointing_button_manager pointing_button_manager_;
};
} // namespace manipulator
} // namespace krbn
