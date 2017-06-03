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

  void reset(void) {
    modifier_flag_manager_.reset();

    pointing_button_manager_.reset();

    // Do not call terminate_virtual_hid_keyboard
    virtual_hid_device_client_.terminate_virtual_hid_pointing();
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
