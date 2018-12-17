#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "iokit_utility.hpp"
#include "pressed_keys_manager.hpp"
#include "types.hpp"
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

namespace krbn {
namespace device_grabber_details {
class entry final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  entry(device_id device_id,
        IOHIDDeviceRef device,
        std::weak_ptr<const core_configuration::core_configuration> core_configuration) : dispatcher_client(),
                                                                                          device_id_(device_id),
                                                                                          core_configuration_(core_configuration),
                                                                                          grabbed_(false),
                                                                                          disabled_(false) {
    device_properties_ = std::make_shared<device_properties>(device_id,
                                                             device);

    pressed_keys_manager_ = std::make_shared<pressed_keys_manager>();
    hid_queue_value_monitor_ = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                          device);
    caps_lock_led_state_manager_ = std::make_shared<krbn::hid_keyboard_caps_lock_led_state_manager>(device);
    device_name_ = iokit_utility::make_device_name_for_log(device_id,
                                                           device);

    hid_queue_value_monitor_->started.connect([this] {
      grabbed_ = true;
      control_caps_lock_led_state_manager();
    });

    hid_queue_value_monitor_->stopped.connect([this] {
      grabbed_ = false;
      control_caps_lock_led_state_manager();
    });
  }

  ~entry(void) {
    detach_from_dispatcher([this] {
      hid_queue_value_monitor_ = nullptr;
      caps_lock_led_state_manager_ = nullptr;
    });
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  void set_core_configuration(std::weak_ptr<const core_configuration::core_configuration> core_configuration) {
    core_configuration_ = core_configuration;

    control_caps_lock_led_state_manager();
  }

  std::shared_ptr<device_properties> get_device_properties(void) const {
    return device_properties_;
  }

  std::shared_ptr<pressed_keys_manager> get_pressed_keys_manager(void) const {
    return pressed_keys_manager_;
  }

  std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor> get_hid_queue_value_monitor(void) const {
    return hid_queue_value_monitor_;
  }

  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> get_caps_lock_led_state_manager(void) const {
    return caps_lock_led_state_manager_;
  }

  const std::string& get_device_name(void) const {
    return device_name_;
  }

  bool get_grabbed(void) const {
    return grabbed_;
  }

  void set_grabbed(bool value) {
    grabbed_ = value;
  }

  bool get_disabled(void) const {
    return disabled_;
  }

  void set_disabled(bool value) {
    disabled_ = value;
  }

  bool is_ignored_device(void) const {
    if (device_properties_) {
      if (auto c = core_configuration_.lock()) {
        if (auto device_identifiers = device_properties_->get_device_identifiers()) {
          return c->get_selected_profile().get_device_ignore(
              *device_identifiers);
        }
      }
    }

    return false;
  }

  bool is_disable_built_in_keyboard_if_exists(void) const {
    if (device_properties_) {
      if (auto c = core_configuration_.lock()) {
        if (auto device_identifiers = device_properties_->get_device_identifiers()) {
          return c->get_selected_profile().get_device_disable_built_in_keyboard_if_exists(
              *device_identifiers);
        }
      }
    }
    return false;
  }

  void async_start_queue_value_monitor(void) const {
    if (hid_queue_value_monitor_) {
      hid_queue_value_monitor_->async_start(kIOHIDOptionsTypeSeizeDevice,
                                            std::chrono::milliseconds(1000));
    }
  }

  void async_stop_queue_value_monitor(void) const {
    if (hid_queue_value_monitor_) {
      hid_queue_value_monitor_->async_stop();
    }
  }

private:
  void control_caps_lock_led_state_manager(void) {
    if (caps_lock_led_state_manager_) {
      if (device_properties_) {
        if (auto c = core_configuration_.lock()) {
          if (auto device_identifiers = device_properties_->get_device_identifiers()) {
            if (c->get_selected_profile().get_device_manipulate_caps_lock_led(*device_identifiers)) {
              if (grabbed_) {
                caps_lock_led_state_manager_->async_start();
                return;
              }
            }
          }
        }
      }

      caps_lock_led_state_manager_->async_stop();
    }
  }

  device_id device_id_;
  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  std::shared_ptr<device_properties> device_properties_;
  std::shared_ptr<pressed_keys_manager> pressed_keys_manager_;
  std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor> hid_queue_value_monitor_;
  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> caps_lock_led_state_manager_;
  std::string device_name_;
  bool grabbed_;
  bool disabled_;
};
} // namespace device_grabber_details
} // namespace krbn
