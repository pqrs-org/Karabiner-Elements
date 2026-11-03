#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "device_utility.hpp"
#include "event_queue.hpp"
#include "game_pad_stick_converter.hpp"
#include "hid_device_events_monitor.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "hid_report_only_events/logitech/hidpp/button.hpp"
#include "hid_report_only_events/logitech/hidpp/device_support.hpp"
#include "iokit_utility.hpp"
#include "logitech_hidpp_session_controller.hpp"
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
  nod::signal<void()> logitech_hidpp_divertable_control_ids_changed;

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
            std::vector<uint8_t> report_descriptor;
            if (!logitech::hidpp::device_support::find_unsupported_device_reason(identifiers)) {
              report_descriptor = hid_device_events_monitor::find_report_descriptor(device);
            }
            logitech_hidpp_button_support_ = logitech::hidpp::device_support(identifiers,
                                                                             report_descriptor);

            if (!logitech_hidpp_button_support_.get_unsupported_reason()) {
              logitech_hidpp_button_report_handler_ =
                  std::make_shared<hid_report_only_events::logitech::hidpp::button::handler>();
            }
          }

          hid_device_events_monitor_ = std::make_shared<hid_device_events_monitor>(
              pqrs::dispatcher::extra::get_shared_dispatcher(),
              pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
              device,
              *device_properties_,
              hid_device_events_monitor::configuration{
                  .enable_input_report_handler = true,
                  // Observe HID++ reports for every supported device, so logitech_hidpp_button can be enabled
                  // by a configuration change without reopening the device.
                  .input_report_handler = logitech_hidpp_button_report_handler_,
              });
          hid_device_events_monitor_->started.connect([this] {
            control_caps_lock_led_state_manager();

            // IOHIDDeviceOpen has succeeded when `started` is emitted, but a
            // Bluetooth device may still reject an output report issued from
            // the same startup turn with kIOReturnNotOpen. Start the HID++
            // session after the monitor and its other started handlers have
            // finished settling.
            enqueue_to_dispatcher(
                [this] {
                  update_logitech_hidpp_button();
                },
                when_now() + std::chrono::milliseconds(100));

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
              abandon_logitech_hidpp_button();
            }
          });
          hid_device_events_monitor_->values_arrived.connect([this](auto&& values_ptr) {
            auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());

            // Check the original values, before they are filtered or transformed.
            notify_logitech_hidpp_button_of_physical_activity(*values_ptr);

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
      if (logitech_hidpp_button_report_handler_) {
        logitech_hidpp_button_report_handler_->clear_input_report_handler();
      }
      logitech_hidpp_session_controller_ = nullptr;
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

    update_logitech_hidpp_button();
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

    auto was_started = hid_device_events_monitor_->is_started();
    auto was_seized = seized();
    auto requested_seized = (options & kIOHIDOptionsTypeSeizeDevice) != 0;

    // Stop the current HID++ session before a reopen closes the device. This
    // also disables queries which have already been queued on run_loop_thread.
    if (was_started && was_seized != requested_seized) {
      stop_logitech_hidpp_button();
    }

    hid_device_events_monitor_->async_start(options,
                                            std::chrono::milliseconds(1000));

    // No `started` signal is emitted if this start merely cancelled a pending
    // stop request without changing the open mode.
    if (was_started && was_seized == requested_seized) {
      update_logitech_hidpp_button();
    }
  }

  void async_stop_hid_device_events_monitor() {
    // Release a held button while the device is still seized,
    // and restore the control before hid_device_events_monitor_ closes the device.
    stop_logitech_hidpp_button();

    hid_device_events_monitor_->async_stop();
  }

  [[nodiscard]] bool seized() const {
    return hid_device_events_monitor_->seized();
  }

  // The physical device has already disappeared, so forget HID++ state
  // without trying to restore controls through the closed IOHIDDevice.
  // This method should be called in the shared dispatcher thread.
  void handle_device_terminated() {
    abandon_logitech_hidpp_button();
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
  // Applies the `logitech_hidpp` setting of the device while the device is seized.
  // This method should be called in the shared dispatcher thread.
  void update_logitech_hidpp_button() {
    std::optional<core_configuration::details::logitech_hidpp> desired;
    if (seized()) {
      auto d = core_configuration_->get_selected_profile().get_device(device_properties_->get_device_identifiers());
      desired = d->get_logitech_hidpp();
    }

    // The output mapping is handled by the Simple Modifications manipulator manager.
    // Keep the HID++ session when only a mapping changes, so the same controls are not
    // restored and diverted again. A session without a selected control is retained for discovery.
    if (logitech_hidpp_session_controller_) {
      const auto desired_control_ids = desired ? desired->get_enabled_control_ids() : std::vector<uint16_t>{};
      const auto current_control_ids = applied_logitech_hidpp_ ? applied_logitech_hidpp_->get_enabled_control_ids() : std::vector<uint16_t>{};
      if (desired_control_ids == current_control_ids) {
        applied_logitech_hidpp_ = desired;
        return;
      }
    }

    stop_logitech_hidpp_button();

    if (!device_ || !logitech_hidpp_button_report_handler_) {
      return;
    }

    applied_logitech_hidpp_ = desired;

    auto log_prefix = fmt::format("{0} logitech_hidpp_button:", device_name_);

    logitech_hidpp_session_controller_ = std::make_shared<logitech_hidpp_session_controller>(pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                                             *device_,
                                                                                             desired ? desired->get_enabled_control_ids() : std::vector<uint16_t>{},
                                                                                             log_prefix);
    logitech_hidpp_session_controller_->divertable_control_ids_changed.connect(
        [this](const auto& control_ids) {
          if (device_properties_->get_logitech_hidpp_divertable_control_ids() != control_ids) {
            device_properties_->set_logitech_hidpp_divertable_control_ids(control_ids);
            logitech_hidpp_divertable_control_ids_changed();
          }
        });
    logitech_hidpp_button_report_handler_->set_input_report_handler(
        [controller = logitech_hidpp_session_controller_.get()](auto report) {
          return controller->handle_input_report(report);
        });
    logitech_hidpp_session_controller_->start();
  }

  // Lets the controller retry after a discovery timeout when the device moves or scrolls again.
  // Only physical motion counts. The logical HID++ button events generated by
  // logitech_hidpp_session_controller must not trigger another discovery attempt.
  // This method should be called in the shared dispatcher thread.
  void notify_logitech_hidpp_button_of_physical_activity(const std::vector<pqrs::osx::iokit_hid_value>& values) {
    if (!logitech_hidpp_session_controller_ ||
        !logitech_hidpp_session_controller_->can_recover_on_physical_activity() ||
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
    auto desired = seized() ? d->get_logitech_hidpp() : std::nullopt;
    if (desired != applied_logitech_hidpp_) {
      return;
    }

    logitech_hidpp_session_controller_->handle_physical_activity();
  }

  // Releases held buttons and restores the controls.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  void stop_logitech_hidpp_button() {
    if (!seized()) {
      // Do not write to a closed device.
      abandon_logitech_hidpp_button();
      return;
    }

    if (logitech_hidpp_session_controller_) {
      for (const auto& event : logitech_hidpp_session_controller_->stop()) {
        post_logitech_hidpp_button_value(event.control_id,
                                         event.pressed,
                                         pqrs::osx::chrono::mach_absolute_time_point());
      }
      logitech_hidpp_button_report_handler_->clear_input_report_handler();
      logitech_hidpp_session_controller_ = nullptr;
    }

    applied_logitech_hidpp_ = std::nullopt;
  }

  // Forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon_logitech_hidpp_button() {
    if (logitech_hidpp_session_controller_) {
      logitech_hidpp_session_controller_->abandon();
      logitech_hidpp_button_report_handler_->clear_input_report_handler();
      logitech_hidpp_session_controller_ = nullptr;
      if (applied_logitech_hidpp_) {
        for (auto control_id : applied_logitech_hidpp_->get_enabled_control_ids()) {
          pressed_keys_manager_->erase(momentary_switch_event(momentary_switch_event_details::logitech_hidpp_button::usage_page,
                                                              pqrs::hid::usage::value_t(control_id)));
        }
      }
    }

    applied_logitech_hidpp_ = std::nullopt;
  }

  void post_logitech_hidpp_button_value(uint16_t control_id,
                                        bool pressed,
                                        absolute_time_point time_stamp) {
    if (!applied_logitech_hidpp_ ||
        !applied_logitech_hidpp_->is_enabled(control_id)) {
      return;
    }

    auto values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>();
    values->emplace_back(time_stamp,
                         pressed ? 1 : 0,
                         momentary_switch_event_details::logitech_hidpp_button::usage_page,
                         pqrs::hid::usage::value_t(control_id),
                         1,
                         0);
    hid_device_events_monitor_->post_input_values(values);
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
  logitech::hidpp::device_support logitech_hidpp_button_support_;
  std::shared_ptr<hid_report_only_events::logitech::hidpp::button::handler> logitech_hidpp_button_report_handler_;
  // The applied `logitech_hidpp` setting while the device is seized, including a refused one.
  std::optional<core_configuration::details::logitech_hidpp> applied_logitech_hidpp_;
  std::shared_ptr<logitech_hidpp_session_controller> logitech_hidpp_session_controller_;
  std::string device_name_;
  std::string device_short_name_;

  // For disabling the built in keyboard
  bool disabled_{false};

  // For ignoring all devices via EventViewer
  bool temporarily_ignore_{false};
};
} // namespace krbn::core_service::daemon::device_grabber_details
