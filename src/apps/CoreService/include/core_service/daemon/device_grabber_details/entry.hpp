#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "device_utility.hpp"
#include "event_queue.hpp"
#include "game_pad_stick_converter.hpp"
#include "hid_device_events_monitor.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "hidpp/device_support.hpp"
#include "hidpp_button_controller.hpp"
#include "iokit_utility.hpp"
#include "pressed_keys_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn::core_service::daemon::device_grabber_details {
class entry final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void(entry&,
                   event_queue::not_null_entries_ptr_t event_queue_entries)>
      hid_values_arrived;

  //
  // Methods
  //

  entry(const entry&) = delete;

  entry(device_id device_id,
        IOHIDDeviceRef device,
        pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration)
      : device_id_(device_id),
        device_(device),
        core_configuration_(core_configuration),
        device_properties_(device_properties::make_device_properties(device_id,
                                                                     device)),
        pressed_keys_manager_(std::make_shared<pressed_keys_manager>()) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
          caps_lock_led_state_manager_ = std::make_shared<krbn::hid_keyboard_caps_lock_led_state_manager>(device);

          {
            const auto& identifiers = device_properties_->get_device_identifiers();
            const auto& transport = device_properties_->get_transport();
            std::vector<uint8_t> report_descriptor;
            if (!hidpp::device_support::find_unsupported_device_reason(identifiers, transport)) {
              report_descriptor = hid_device_events_monitor::find_report_descriptor(device);
            }
            hidpp_button_support_ = hidpp::device_support(identifiers,
                                                          transport,
                                                          report_descriptor);
          }

          hid_device_events_monitor_ = std::make_shared<hid_device_events_monitor>(
              pqrs::dispatcher::extra::get_shared_dispatcher(),
              pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
              device,
              *device_properties_,
              hid_device_events_monitor::configuration{
                  .enable_input_report_handler = true,
                  // HID++ reports are observed for every supported device, so hidpp_button can be enabled
                  // by a configuration change without reopening the device.
                  .vendor_input_report_filter = hidpp_button_support_.get_unsupported_reason()
                                                    ? nullptr
                                                    : hidpp::is_long_report,
              });
          hid_device_events_monitor_->started.connect([this] {
            control_caps_lock_led_state_manager();

            update_hidpp_button();

            if (seized()) {
              if (device_properties_->get_device_identifiers().get_is_game_pad()) {
                game_pad_stick_converter_ = std::make_unique<game_pad_stick_converter>(device_properties_,
                                                                                       core_configuration_);
                game_pad_stick_converter_->pointing_motion_arrived.connect([this](auto&& event_queue_entry) {
                  auto event_queue_entries = std::make_shared<std::vector<event_queue::not_null_const_entry_ptr_t>>();
                  event_queue_entries->push_back(event_queue_entry);

                  hid_values_arrived(*this,
                                     event_queue_entries);
                });
              }
            }
          });
          hid_device_events_monitor_->stopped.connect([this] {
            control_caps_lock_led_state_manager();

            game_pad_stick_converter_ = nullptr;

            // The device is already closed, so the control cannot be restored here.
            // The device resets temporary diversion when it reconnects, and a held button is released by device_ungrabbed.
            // If the device has already been opened again, this signal is stale; the diversion is still in effect, so the
            // current session is kept.
            if (!seized()) {
              abandon_hidpp_button();
            }
          });
          hid_device_events_monitor_->vendor_input_report_arrived.connect([this](auto&& report_id,
                                                                                 auto&& report,
                                                                                 auto&& time_stamp) {
            if (hidpp_button_controller_) {
              hidpp_button_controller_->handle_input_report(report, time_stamp);
            }
          });
          hid_device_events_monitor_->values_arrived.connect([this](auto&& values_ptr) {
            auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());

            // Check the original values, before they are filtered or transformed.
            notify_hidpp_button_of_physical_activity(*values_ptr);

            auto hid_values = *values_ptr;

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
                                                                          {
                                                                              .pointing_motion_xy_multiplier = d->get_pointing_motion_xy_multiplier(),
                                                                              .pointing_motion_wheels_multiplier = d->get_pointing_motion_wheels_multiplier(),
                                                                          });

            event_queue_entries = event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue_entries,
                                                                                                                   device_id_,
                                                                                                                   pressed_keys_manager_);
            hid_values_arrived(*this,
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
        });
  }

  ~entry() {
    detach_from_dispatcher([this] {
      // Restore the control before hid_device_events_monitor_ closes the device.
      hidpp_button_controller_ = nullptr;
      hid_device_events_monitor_ = nullptr;
      game_pad_stick_converter_ = nullptr;
      caps_lock_led_state_manager_ = nullptr;
    });
  }

  [[nodiscard]] device_id get_device_id() const {
    return device_id_;
  }

  // This method should be called in the shared dispatcher thread.
  void set_core_configuration(pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration) {
    core_configuration_ = core_configuration;

    control_caps_lock_led_state_manager();

    if (game_pad_stick_converter_) {
      game_pad_stick_converter_->set_core_configuration(core_configuration);
    }

    update_hidpp_button();
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<device_properties> get_device_properties() const {
    return device_properties_;
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<pressed_keys_manager> get_pressed_keys_manager() const {
    return pressed_keys_manager_;
  }

  [[nodiscard]] std::shared_ptr<hid_device_events_monitor> get_hid_device_events_monitor() const {
    return hid_device_events_monitor_;
  }

  void set_caps_lock_led_state(std::optional<led_state> state) {
    caps_lock_led_state_manager_->set_state(state);
  }

  [[nodiscard]] const std::string& get_device_name() const {
    return device_name_;
  }

  [[nodiscard]] const std::string& get_device_short_name() const {
    return device_short_name_;
  }

  [[nodiscard]] bool get_disabled() const {
    return disabled_;
  }

  void set_disabled(bool value) {
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return;
    }

    disabled_ = value;
  }

  [[nodiscard]] bool get_temporarily_ignore() const {
    return temporarily_ignore_;
  }

  void set_temporarily_ignore(bool value) {
    temporarily_ignore_ = value;
  }

  [[nodiscard]] bool is_disable_built_in_keyboard_if_exists() const {
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

  [[nodiscard]] bool determine_is_built_in_keyboard() const {
    auto device = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    return device_utility::determine_is_built_in_keyboard(*device, *device_properties_);
  }

  void async_start_hid_device_events_monitor(grabbable_state::state state) {
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

        case grabbable_state::state::ungrabbable:
        case grabbable_state::state::none:
        case grabbable_state::state::end_:
          // Do not start hid_device_events_monitor_.
          return;
      }
    }

    //
    // Start
    //

    hid_device_events_monitor_->async_start(options,
                                            std::chrono::milliseconds(1000));

    // The device may still be seized if this start cancelled a pending stop request.
    update_hidpp_button();
  }

  void async_stop_hid_device_events_monitor() {
    // Release a held button while the device is still seized,
    // and restore the control before hid_device_events_monitor_ closes the device.
    stop_hidpp_button();

    hid_device_events_monitor_->async_stop();
  }

  [[nodiscard]] bool seized() const {
    return hid_device_events_monitor_->seized();
  }

  [[nodiscard]] bool needs_to_observe_device() const {
    // We must monitor the {pqrs::hid::usage_page::leds, pqrs::hid::usage::led::caps_lock} event from the virtual HID keyboard to manage the caps lock LED on physical keyboards.
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return true;
    }

    return false;
  }

  // Return whether the device is a target for modifying input events.
  [[nodiscard]] bool needs_to_seize_device() const {
    if (device_properties_->get_device_identifiers().get_is_virtual_device()) {
      return false;
    }

    if (temporarily_ignore_) {
      return false;
    }

    // We have to seize the device in order to discard all input events.
    if (disabled_) {
      return true;
    }

    auto device = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    return !device_utility::determine_should_ignore_device(*device,
                                                           *device_properties_);
  }

private:
  // Applies the `hidpp_button` setting of the device while the device is seized.
  // This method should be called in the shared dispatcher thread.
  void update_hidpp_button() {
    std::optional<core_configuration::details::hidpp_button> desired;
    if (seized()) {
      auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
      desired = d->get_hidpp_button();
    }

    // Keep the current session while the setting is unchanged, so unrelated configuration changes do not
    // restore and divert the control again.
    if (desired == hidpp_button_) {
      return;
    }

    stop_hidpp_button();

    if (!desired) {
      return;
    }

    hidpp_button_ = desired;

    auto log_prefix = fmt::format("{0} hidpp_button:", device_name_);

    if (auto reason = hidpp_button_support_.find_pointing_button_conflict(desired->get_pointing_button())) {
      logger::get_logger()->warn("{0} not activated: {1}", log_prefix, *reason);
      return;
    }

    if (!device_) {
      return;
    }

    hidpp_button_controller_ = std::make_unique<hidpp_button_controller>(pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                         *device_,
                                                                         desired->get_control_id(),
                                                                         log_prefix);
    hidpp_button_controller_->pressed_changed.connect([this,
                                                       pointing_button = desired->get_pointing_button()](auto&& pressed,
                                                                                                         auto&& time_stamp) {
      // Report the button through the same path as physical buttons.
      auto values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>();
      values->emplace_back(time_stamp,
                           pressed ? 1 : 0,
                           pqrs::hid::usage_page::button,
                           pointing_button,
                           1,
                           0);
      hid_device_events_monitor_->post_input_values(values);
    });
    hidpp_button_controller_->start();
  }

  // Lets the controller retry after a discovery timeout when the device moves or scrolls again.
  // Only motion counts: button values and vendor reports include the buttons which hidpp_button_controller posts.
  // This method should be called in the shared dispatcher thread.
  void notify_hidpp_button_of_physical_activity(const std::vector<pqrs::osx::iokit_hid_value>& values) {
    if (!hidpp_button_controller_ ||
        !hidpp_button_controller_->can_recover_on_physical_activity() ||
        !seized() ||
        disabled_ ||
        !needs_to_seize_device()) {
      return;
    }

    auto moved = std::any_of(values.begin(),
                             values.end(),
                             [](const auto& v) {
                               return v.get_integer_value() != 0 &&
                                      (v.conforms_to(pqrs::hid::usage_page::generic_desktop, pqrs::hid::usage::generic_desktop::x) ||
                                       v.conforms_to(pqrs::hid::usage_page::generic_desktop, pqrs::hid::usage::generic_desktop::y) ||
                                       v.conforms_to(pqrs::hid::usage_page::generic_desktop, pqrs::hid::usage::generic_desktop::wheel) ||
                                       v.conforms_to(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::ac_pan));
                             });
    if (!moved) {
      return;
    }

    // Ignore a pending configuration change which the controller has not applied yet.
    auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
    if (d->get_hidpp_button() != hidpp_button_) {
      return;
    }

    hidpp_button_controller_->handle_physical_activity();
  }

  // Releases a held button and restores the control.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  void stop_hidpp_button() {
    if (!seized()) {
      // Do not write to a closed device.
      abandon_hidpp_button();
      return;
    }

    if (hidpp_button_controller_) {
      hidpp_button_controller_->stop();
      hidpp_button_controller_ = nullptr;
    }

    hidpp_button_ = std::nullopt;
  }

  // Forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon_hidpp_button() {
    if (hidpp_button_controller_) {
      hidpp_button_controller_->abandon();
      hidpp_button_controller_ = nullptr;
      pressed_keys_manager_->erase(momentary_switch_event(pqrs::hid::usage_page::button,
                                                          hidpp_button_->get_pointing_button()));
    }

    hidpp_button_ = std::nullopt;
  }

  void control_caps_lock_led_state_manager() {
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
  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration_;
  pqrs::not_null_shared_ptr_t<device_properties> device_properties_;
  pqrs::not_null_shared_ptr_t<pressed_keys_manager> pressed_keys_manager_;
  std::shared_ptr<hid_keyboard_caps_lock_led_state_manager> caps_lock_led_state_manager_;
  std::shared_ptr<hid_device_events_monitor> hid_device_events_monitor_;
  std::unique_ptr<game_pad_stick_converter> game_pad_stick_converter_;
  hidpp::device_support hidpp_button_support_;
  // The applied `hidpp_button` setting while the device is seized, including a refused one.
  std::optional<core_configuration::details::hidpp_button> hidpp_button_;
  std::unique_ptr<hidpp_button_controller> hidpp_button_controller_;
  std::string device_name_;
  std::string device_short_name_;

  // For disabling the built in keyboard
  bool disabled_{false};

  // For ignoring all devices via EventViewer
  bool temporarily_ignore_{false};
};
} // namespace krbn::core_service::daemon::device_grabber_details
