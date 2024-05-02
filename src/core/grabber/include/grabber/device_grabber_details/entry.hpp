#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "device_utility.hpp"
#include "event_queue.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "hid_queue_values_converter.hpp"
#include "iokit_utility.hpp"
#include "pressed_keys_manager.hpp"
#include "probable_stuck_events_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "types.hpp"
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

namespace krbn {
namespace grabber {
namespace device_grabber_details {
class entry final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void(entry&,
                   std::shared_ptr<event_queue::queue> event_queue,
                   const std::vector<pqrs::osx::iokit_hid_value>& hid_values)>
      hid_queue_values_arrived;

  //
  // Methods
  //

  entry(const entry&) = delete;

  entry(device_id device_id,
        IOHIDDeviceRef device,
        std::weak_ptr<const core_configuration::core_configuration> core_configuration) : dispatcher_client(),
                                                                                          device_id_(device_id),
                                                                                          core_configuration_(core_configuration),
                                                                                          disabled_(false) {
    device_properties_ = device_properties(device_id,
                                           device);

    probable_stuck_events_manager_ = std::make_shared<probable_stuck_events_manager>();

    pressed_keys_manager_ = std::make_shared<pressed_keys_manager>();

    caps_lock_led_state_manager_ = std::make_shared<krbn::hid_keyboard_caps_lock_led_state_manager>(device);

    hid_queue_value_monitor_ = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                          pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                                          device);
    hid_queue_value_monitor_->started.connect([this] {
      control_caps_lock_led_state_manager();

      if (seized()) {
        logger::get_logger()->info("{0} probable_stuck_events_manager_->clear()",
                                   device_name_);

        probable_stuck_events_manager_->clear();
      }
    });
    hid_queue_value_monitor_->stopped.connect([this] {
      control_caps_lock_led_state_manager();
    });
    hid_queue_value_monitor_->values_arrived.connect([this](auto&& values_ptr) {
      auto hid_values = hid_queue_values_converter_.make_hid_values(device_id_,
                                                                    values_ptr);
      auto event_queue = event_queue::utility::make_queue(device_properties_,
                                                          hid_values);

      event_queue = event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue,
                                                                                                     device_id_,
                                                                                                     pressed_keys_manager_);
      hid_queue_values_arrived(*this,
                               event_queue,
                               hid_values);
    });

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

    //
    // Update caps_lock_led_state_manager state
    //

    control_caps_lock_led_state_manager();
  }

  const device_properties& get_device_properties(void) const {
    return device_properties_;
  }

  std::shared_ptr<probable_stuck_events_manager> get_probable_stuck_events_manager(void) const {
    return probable_stuck_events_manager_;
  }

  std::shared_ptr<pressed_keys_manager> get_pressed_keys_manager(void) const {
    return pressed_keys_manager_;
  }

  std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor> get_hid_queue_value_monitor(void) const {
    return hid_queue_value_monitor_;
  }

  void set_caps_lock_led_state(std::optional<led_state> state) {
    caps_lock_led_state_manager_->set_state(state);
  }

  const std::string& get_device_name(void) const {
    return device_name_;
  }

  const std::string& get_device_short_name(void) const {
    return device_short_name_;
  }

  bool get_disabled(void) const {
    return disabled_;
  }

  void set_disabled(bool value) {
    if (is_karabiner_virtual_hid_device()) {
      return;
    }

    disabled_ = value;
  }

  bool is_disable_built_in_keyboard_if_exists(void) const {
    if (is_karabiner_virtual_hid_device()) {
      return false;
    }

    if (device_properties_.get_is_built_in_keyboard() ||
        device_properties_.get_is_built_in_pointing_device() ||
        device_properties_.get_is_built_in_touch_bar()) {
      return false;
    }

    if (auto c = core_configuration_.lock()) {
      return c->get_selected_profile().get_device_disable_built_in_keyboard_if_exists(
          device_properties_.get_device_identifiers());
    }

    return false;
  }

  bool determine_is_built_in_keyboard(void) const {
    if (auto c = core_configuration_.lock()) {
      return device_utility::determine_is_built_in_keyboard(*c, device_properties_);
    }

    return false;
  }

  void async_start_queue_value_monitor(grabbable_state::state state) {
    auto options = kIOHIDOptionsTypeNone;

    if (is_karabiner_virtual_hid_device()) {
      options = kIOHIDOptionsTypeNone;
    } else {
      switch (state) {
        case grabbable_state::state::grabbable:
          if (needs_to_seize_device()) {
            options = kIOHIDOptionsTypeSeizeDevice;
          }
          break;

        case grabbable_state::state::ungrabbable_temporarily:
          options = kIOHIDOptionsTypeNone;
          break;

        case grabbable_state::state::ungrabbable_permanently:
        case grabbable_state::state::none:
        case grabbable_state::state::end_:
          // Do not start hid_queue_value_monitor_.
          return;
      }
    }

    //
    // Start
    //

    hid_queue_value_monitor_->async_start(options,
                                          std::chrono::milliseconds(1000));
  }

  void async_stop_queue_value_monitor(void) {
    hid_queue_value_monitor_->async_stop();
  }

  bool seized(void) const {
    return hid_queue_value_monitor_->seized();
  }

  bool is_karabiner_virtual_hid_device(void) const {
    if (auto v = device_properties_.get_is_karabiner_virtual_hid_device()) {
      return *v;
    }

    return false;
  }

  bool needs_to_observe_device(void) const {
    // We must monitor the {pqrs::hid::usage_page::leds, pqrs::hid::usage::led::caps_lock} event from the virtual HID keyboard to manage the caps lock LED on physical keyboards.
    if (is_karabiner_virtual_hid_device()) {
      return true;
    }

    return false;
  }

  // Return whether the device is a target for modifying input events.
  bool needs_to_seize_device(void) const {
    if (is_karabiner_virtual_hid_device()) {
      return false;
    }

    // We have to seize the device in order to discard all input events.
    if (disabled_) {
      return true;
    }

    if (auto c = core_configuration_.lock()) {
      return !(c->get_selected_profile().get_device_ignore(device_properties_.get_device_identifiers()));
    }

    return false;
  }

private:
  void control_caps_lock_led_state_manager(void) {
    if (is_karabiner_virtual_hid_device()) {
      return;
    }

    if (caps_lock_led_state_manager_) {
      if (auto c = core_configuration_.lock()) {
        if (c->get_selected_profile().get_device_manipulate_caps_lock_led(device_properties_.get_device_identifiers())) {
          if (seized()) {
            caps_lock_led_state_manager_->async_start();
            return;
          }
        }
      }

      caps_lock_led_state_manager_->async_stop();
    }
  }

  device_id device_id_;
  std::weak_ptr<const core_configuration::core_configuration> core_configuration_;
  device_properties device_properties_;
  std::shared_ptr<probable_stuck_events_manager> probable_stuck_events_manager_;
  std::shared_ptr<pressed_keys_manager> pressed_keys_manager_;
  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> caps_lock_led_state_manager_;
  std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor> hid_queue_value_monitor_;
  hid_queue_values_converter hid_queue_values_converter_;
  std::string device_name_;
  std::string device_short_name_;

  bool disabled_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
