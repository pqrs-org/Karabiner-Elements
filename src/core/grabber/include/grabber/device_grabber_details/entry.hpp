#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "event_queue.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "iokit_utility.hpp"
#include "pressed_keys_manager.hpp"
#include "types.hpp"
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

namespace krbn {
namespace grabber {
namespace device_grabber_details {
class entry final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  entry(device_id device_id,
        IOHIDDeviceRef device,
        std::weak_ptr<const core_configuration::core_configuration> core_configuration) : dispatcher_client(),
                                                                                          device_id_(device_id),
                                                                                          core_configuration_(core_configuration),
                                                                                          hid_queue_value_monitor_async_start_called_(false),
                                                                                          first_value_arrived_(false),
                                                                                          grabbed_(false),
                                                                                          disabled_(false),
                                                                                          grabbed_time_stamp_(0),
                                                                                          ungrabbed_time_stamp_(0) {
    device_properties_ = std::make_shared<device_properties>(device_id,
                                                             device);

    pressed_keys_manager_ = std::make_shared<pressed_keys_manager>();
    hid_queue_value_monitor_ = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                          device);
    caps_lock_led_state_manager_ = std::make_shared<krbn::hid_keyboard_caps_lock_led_state_manager>(device);
    device_name_ = iokit_utility::make_device_name_for_log(device_id,
                                                           device);
    device_short_name_ = iokit_utility::make_device_name(device);
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

  bool get_first_value_arrived(void) const {
    return first_value_arrived_;
  }

  void set_first_value_arrived(bool value) {
    first_value_arrived_ = value;
  }

  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> get_caps_lock_led_state_manager(void) const {
    return caps_lock_led_state_manager_;
  }

  const std::string& get_device_name(void) const {
    return device_name_;
  }

  const std::string& get_device_short_name(void) const {
    return device_short_name_;
  }

  bool get_grabbed(void) const {
    return grabbed_;
  }

  void set_grabbed(bool value) {
    grabbed_ = value;

    if (grabbed_) {
      grabbed_time_stamp_ = pqrs::osx::chrono::mach_absolute_time_point();
    } else {
      ungrabbed_time_stamp_ = pqrs::osx::chrono::mach_absolute_time_point();
    }

    control_caps_lock_led_state_manager();
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

  void async_start_queue_value_monitor(void) {
    if (hid_queue_value_monitor_) {
      if (!hid_queue_value_monitor_async_start_called_) {
        first_value_arrived_ = false;
        hid_queue_value_monitor_async_start_called_ = true;
      }

      hid_queue_value_monitor_->async_start(kIOHIDOptionsTypeSeizeDevice,
                                            std::chrono::milliseconds(1000));
    }
  }

  void async_stop_queue_value_monitor(void) {
    if (hid_queue_value_monitor_) {
      hid_queue_value_monitor_async_start_called_ = false;

      hid_queue_value_monitor_->async_stop();
    }
  }

  bool is_grabbed(absolute_time_point time_stamp) {
    if (grabbed_) {
      if (grabbed_time_stamp_ <= time_stamp) {
        return true;
      }
    } else {
      //
      // (grabbed_time_stamp_ <= ungrabbed_time_stamp_) when (grabbed_ == false)
      //

      if (grabbed_time_stamp_ <= time_stamp &&
          time_stamp <= ungrabbed_time_stamp_) {
        return true;
      }
    }

    return false;
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
  bool hid_queue_value_monitor_async_start_called_;
  bool first_value_arrived_;

  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> caps_lock_led_state_manager_;
  std::string device_name_;
  std::string device_short_name_;

  bool grabbed_;
  bool disabled_;

  absolute_time_point grabbed_time_stamp_;
  absolute_time_point ungrabbed_time_stamp_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
