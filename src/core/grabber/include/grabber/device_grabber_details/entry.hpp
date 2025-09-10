#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "device_utility.hpp"
#include "event_queue.hpp"
#include "game_pad_stick_converter.hpp"
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
                   event_queue::not_null_entries_ptr_t event_queue_entries)>
      hid_queue_values_arrived;

  //
  // Methods
  //

  entry(const entry&) = delete;

  entry(device_id device_id,
        IOHIDDeviceRef device,
        gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration)
      : dispatcher_client(),
        device_id_(device_id),
        core_configuration_(core_configuration),
        device_properties_(device_properties::make_device_properties(device_id,
                                                                     device)),
        disabled_(false) {
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

        if (device_properties_->get_device_identifiers().get_is_game_pad()) {
          game_pad_stick_converter_ = std::make_unique<game_pad_stick_converter>(device_properties_,
                                                                                 core_configuration_);
          game_pad_stick_converter_->pointing_motion_arrived.connect([this](auto&& event_queue_entry) {
            auto event_queue_entries = std::make_shared<std::vector<gsl::not_null<std::shared_ptr<const event_queue::entry>>>>();
            event_queue_entries->push_back(event_queue_entry);

            hid_queue_values_arrived(*this,
                                     event_queue_entries);
          });
        }
      }
    });
    hid_queue_value_monitor_->stopped.connect([this] {
      control_caps_lock_led_state_manager();

      game_pad_stick_converter_ = nullptr;
    });
    hid_queue_value_monitor_->values_arrived.connect([this](auto&& values_ptr) {
      auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());

      auto hid_values = hid_queue_values_converter_.make_hid_values(device_id_,
                                                                    values_ptr);

      //
      // Eliminated the entries that needed to be removed from hid_values
      //

      std::erase_if(hid_values,
                    [this, &d](const auto& v) {
                      //
                      // Handle ignore_vendor_events
                      //

                      // For Apple devices, process vendor events regardless of the "ignore_vendor_events" setting.
                      // Even if karabiner.json is manually edited to set "ignore_vendor_events": true,
                      // ignore that setting and handle vendor events.
                      if (d->get_ignore_vendor_events() &&
                          !device_properties_->get_is_apple()) {
                        // 0xff
                        if (v.get_usage_page() == pqrs::hid::usage_page::apple_vendor_top_case) {
                          return true;
                        }

                        // Vendor-defined (0xff00-0xffff)
                        if (v.get_usage_page() >= pqrs::hid::usage_page::value_t(0xff00) &&
                            v.get_usage_page() <= pqrs::hid::usage_page::value_t(0xffff)) {
                          return true;
                        }
                      }

                      //
                      // Filter useless events
                      //

                      if (core_configuration_->get_global_configuration().get_filter_useless_events_from_specific_devices()) {
                        if (device_properties_->get_device_identifiers().is_nintendo_pro_controller_0x057e_0x2009() &&
                            device_properties_->get_transport() == "USB") {
                          // Nintendo's Pro Controller, when connected via USB, generates a high frequency of events even when no input is made.
                          // As these events contain no meaningful information, they should be ignored.

                          // Since button on/off events keep firing endlessly, they must be ignored.
                          if (v.get_usage_page() == pqrs::hid::usage_page::button) {
                            return true;
                          }

                          // The sticks continuously move randomly, they must be ignored.
                          if (v.get_usage_page() == pqrs::hid::usage_page::generic_desktop &&
                              (v.get_usage() == pqrs::hid::usage::generic_desktop::x ||
                               v.get_usage() == pqrs::hid::usage::generic_desktop::y ||
                               v.get_usage() == pqrs::hid::usage::generic_desktop::z ||
                               v.get_usage() == pqrs::hid::usage::generic_desktop::rz)) {
                            return true;
                          }
                        }
                      }

                      return false;
                    });

      //
      // Make event queue
      //

      auto event_queue_entries = event_queue::utility::make_entries(device_properties_,
                                                                    hid_values,
                                                                    event_queue::utility::make_queue_parameters{
                                                                        .pointing_motion_xy_multiplier = d->get_pointing_motion_xy_multiplier(),
                                                                        .pointing_motion_wheels_multiplier = d->get_pointing_motion_wheels_multiplier(),
                                                                    });

      event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue_entries,
                                                                                       device_id_,
                                                                                       pressed_keys_manager_);
      hid_queue_values_arrived(*this,
                               event_queue_entries);

      //
      // game pad stick to pointing motion
      //

      if (game_pad_stick_converter_) {
        game_pad_stick_converter_->convert(hid_values);
      }
    });

    device_name_ = iokit_utility::make_device_name_for_log(device_id,
                                                           device);
    device_short_name_ = iokit_utility::make_device_name(device);
  }

  ~entry(void) {
    detach_from_dispatcher([this] {
      hid_queue_value_monitor_ = nullptr;
      game_pad_stick_converter_ = nullptr;
      caps_lock_led_state_manager_ = nullptr;
    });
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  // This method should be called in the shared dispatcher thread.
  void set_core_configuration(gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration) {
    core_configuration_ = core_configuration;

    control_caps_lock_led_state_manager();

    if (game_pad_stick_converter_) {
      game_pad_stick_converter_->set_core_configuration(core_configuration);
    }
  }

  gsl::not_null<std::shared_ptr<device_properties>> get_device_properties(void) const {
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
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return;
    }

    disabled_ = value;
  }

  bool is_disable_built_in_keyboard_if_exists(void) const {
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return false;
    }

    if (device_properties_->get_is_built_in_keyboard() ||
        device_properties_->get_is_built_in_pointing_device() ||
        device_properties_->get_is_built_in_touch_bar()) {
      return false;
    }

    auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    return d->get_disable_built_in_keyboard_if_exists();
  }

  bool determine_is_built_in_keyboard(void) const {
    return device_utility::determine_is_built_in_keyboard(*core_configuration_, *device_properties_);
  }

  void async_start_queue_value_monitor(grabbable_state::state state) {
    auto options = kIOHIDOptionsTypeNone;

    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
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

  bool needs_to_observe_device(void) const {
    // We must monitor the {pqrs::hid::usage_page::leds, pqrs::hid::usage::led::caps_lock} event from the virtual HID keyboard to manage the caps lock LED on physical keyboards.
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return true;
    }

    return false;
  }

  // Return whether the device is a target for modifying input events.
  bool needs_to_seize_device(void) const {
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return false;
    }

    // We have to seize the device in order to discard all input events.
    if (disabled_) {
      return true;
    }

    auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    return !(d->get_ignore());
  }

private:
  void control_caps_lock_led_state_manager(void) {
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return;
    }

    if (caps_lock_led_state_manager_) {
      auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
      if (d->get_manipulate_caps_lock_led()) {
        if (seized()) {
          caps_lock_led_state_manager_->async_start();
          return;
        }
      }

      caps_lock_led_state_manager_->async_stop();
    }
  }

  device_id device_id_;
  gsl::not_null<std::shared_ptr<const core_configuration::core_configuration>> core_configuration_;
  gsl::not_null<std::shared_ptr<device_properties>> device_properties_;
  std::shared_ptr<probable_stuck_events_manager> probable_stuck_events_manager_;
  std::shared_ptr<pressed_keys_manager> pressed_keys_manager_;
  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> caps_lock_led_state_manager_;
  std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor> hid_queue_value_monitor_;
  std::unique_ptr<game_pad_stick_converter> game_pad_stick_converter_;
  hid_queue_values_converter hid_queue_values_converter_;
  std::string device_name_;
  std::string device_short_name_;

  bool disabled_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
