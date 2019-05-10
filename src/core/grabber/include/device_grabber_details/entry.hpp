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
                                                                                          orphan_key_up_keys_last_time_stamp_(0),
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

    key_down_arrived_keys_manager_ = std::make_shared<pressed_keys_manager>();
    orphan_key_up_keys_manager_ = std::make_shared<pressed_keys_manager>();

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

  std::shared_ptr<pressed_keys_manager> get_orphan_key_up_keys_manager(void) const {
    return orphan_key_up_keys_manager_;
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

  void update_orphan_key_up_keys(const key_down_up_valued_event& e,
                                 event_type t,
                                 absolute_time_point time_stamp) {
    // Skip old event.
    if (time_stamp < orphan_key_up_keys_last_time_stamp_) {
      return;
    }

    orphan_key_up_keys_last_time_stamp_ = time_stamp;

    switch (t) {
      case event_type::key_down:
        key_down_arrived_keys_manager_->insert(e);
        break;

      case event_type::key_up:
        if (!key_down_arrived_keys_manager_->exists(e)) {
          orphan_key_up_keys_manager_->insert(e);
        } else {
          orphan_key_up_keys_manager_->erase(e);
        }
        break;

      case event_type::single:
        // Do nothing
        break;
    }
  }

  void clear_orphan_key_up_keys(void) {
    orphan_key_up_keys_last_time_stamp_ = absolute_time_point(0);
    key_down_arrived_keys_manager_->clear();
    orphan_key_up_keys_manager_->clear();
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

  absolute_time_point orphan_key_up_keys_last_time_stamp_;
  std::shared_ptr<pressed_keys_manager> key_down_arrived_keys_manager_;
  std::shared_ptr<pressed_keys_manager> orphan_key_up_keys_manager_;

  bool grabbed_;
  bool disabled_;
};
} // namespace device_grabber_details
} // namespace krbn
