#pragma once

#include "core_configuration/core_configuration.hpp"
#include "hid_report_only_events/logitech/hidpp/button.hpp"
#include "logitech_hidpp_session_controller.hpp"
#include "manipulator/types/to_event_definition.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn::core_service::daemon::device_grabber_details {
class logitech_hidpp_button_manager final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  nod::signal<void(const std::vector<uint16_t>&)> divertable_control_ids_changed;
  nod::signal<void(uint16_t)> control_abandoned;
  nod::signal<void(pqrs::not_null_shared_ptr_t<std::vector<pqrs::osx::iokit_hid_value>>)> hid_values_generated;

  logitech_hidpp_button_manager(const logitech_hidpp_button_manager&) = delete;

  logitech_hidpp_button_manager(pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                                IOHIDDeviceRef device,
                                const device_identifiers& identifiers,
                                const std::string& log_prefix)
      : dispatcher_client(pqrs::dispatcher::extra::get_shared_dispatcher()),
        run_loop_thread_(run_loop_thread),
        device_(device),
        identifiers_(identifiers),
        log_prefix_(log_prefix) {
    dispatcher_client_constructor_guard_.initialize([] {});
  }

  ~logitech_hidpp_button_manager() override {
    detach_from_dispatcher([this] {
      session_controller_ = nullptr;
    });
  }

  [[nodiscard]] std::vector<logitech::hidpp::session::result::button_event> handle_input_report(
      std::span<const uint8_t> report) {
    if (session_controller_) {
      return session_controller_->handle_input_report(report);
    }
    return {};
  }

  // Applies the desired setting. A session without a selected control is retained for discovery.
  // This method should be called in the shared dispatcher thread while the device is open.
  void update(pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration,
              bool device_is_seized) {
    apply(make_desired_configuration(*core_configuration,
                                     device_is_seized),
          device_is_seized);
  }

  // Applies the desired setting after the device has finished settling.
  void async_update(pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration,
                    bool device_is_seized) {
    // A Bluetooth device may reject an output report with kIOReturnNotOpen immediately
    // after IOHIDDeviceOpen succeeds. Give the device time to become writable before
    // starting the HID++ session.
    update_task_.debounce_after(
        [this, core_configuration, device_is_seized] {
          update(core_configuration,
                 device_is_seized);
        },
        std::chrono::milliseconds(100));
  }

  void cancel_pending_update() {
    update_task_.cancel();
  }

  // Releases held buttons and restores diverted controls if the device can still be written.
  // Otherwise, forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread.
  void stop(bool device_is_seized) {
    update_task_.cancel();

    if (!device_is_seized) {
      abandon();
      return;
    }

    if (session_controller_) {
      std::vector<logitech::hidpp::session::result::button_event> button_events;
      for (const auto& event : session_controller_->stop()) {
        if (applied_configuration_ &&
            applied_configuration_->is_enabled(event.control_id)) {
          button_events.push_back(event);
        }
      }

      // Emit logical key_up events for controls which are still held, so their
      // mapped output keys are not left pressed when the HID++ session stops.
      if (!button_events.empty()) {
        auto values = hid_report_only_events::logitech::hidpp::button::make_hid_values(
            button_events,
            pqrs::osx::chrono::mach_absolute_time_point());
        hid_values_generated(std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>(
            std::move(values)));
      }

      session_controller_ = nullptr;
    }

    applied_configuration_ = std::nullopt;
    applied_control_ids_.clear();
  }

  // Forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon() {
    update_task_.cancel();

    if (session_controller_) {
      session_controller_->abandon();
      session_controller_ = nullptr;

      if (applied_configuration_) {
        for (auto control_id : applied_configuration_->get_enabled_control_ids()) {
          control_abandoned(control_id);
        }
      }
    }

    applied_configuration_ = std::nullopt;
    applied_control_ids_.clear();
  }

  [[nodiscard]] bool can_recover_on_physical_activity() const {
    return session_controller_ &&
           session_controller_->can_recover_on_physical_activity();
  }

  void handle_physical_activity() {
    if (session_controller_) {
      session_controller_->handle_physical_activity();
    }
  }

  [[nodiscard]] bool configuration_is_applied(
      pqrs::not_null_shared_ptr_t<const core_configuration::core_configuration> core_configuration,
      bool device_is_seized) const {
    return make_desired_configuration(*core_configuration,
                                      device_is_seized) == applied_configuration_;
  }

private:
  [[nodiscard]] std::vector<uint16_t> make_validated_enabled_control_ids(
      const std::optional<core_configuration::details::logitech_hidpp>& configuration) const {
    std::vector<uint16_t> result;

    if (configuration) {
      for (const auto& button : configuration->get_buttons()) {
        if (!button.enabled) {
          continue;
        }

        // Validate every `to` entry before diverting the control, so an invalid
        // output mapping does not suppress the physical button input.
        try {
          for (const auto& value : button.to) {
            [[maybe_unused]] const manipulator::to_event_definition definition(value);
          }
          result.push_back(button.control_id);
        } catch (const std::exception& e) {
          logger::get_logger()->error(fmt::format("{0} ignoring invalid mapping for control_id {1}: {2}",
                                                  log_prefix_,
                                                  button.control_id,
                                                  e.what()));
        }
      }
    }

    return result;
  }

  [[nodiscard]] std::optional<core_configuration::details::logitech_hidpp> make_desired_configuration(
      const core_configuration::core_configuration& core_configuration,
      bool device_is_seized) const {
    if (device_is_seized) {
      return core_configuration.get_selected_profile().get_device(identifiers_)->get_logitech_hidpp();
    }
    return std::nullopt;
  }

  void apply(const std::optional<core_configuration::details::logitech_hidpp>& desired,
             bool device_is_seized) {
    update_task_.cancel();

    auto desired_control_ids = make_validated_enabled_control_ids(desired);

    // The output mapping is handled by the Simple Modifications manipulator manager.
    // Keep the HID++ session when only a mapping changes, so the same controls are not
    // restored and diverted again.
    if (session_controller_) {
      if (desired_control_ids == applied_control_ids_) {
        applied_configuration_ = desired;
        return;
      }
    }

    stop(device_is_seized);

    applied_configuration_ = desired;
    applied_control_ids_ = desired_control_ids;

    session_controller_ = std::make_shared<logitech_hidpp_session_controller>(run_loop_thread_,
                                                                              *device_,
                                                                              desired_control_ids,
                                                                              log_prefix_);
    session_controller_->divertable_control_ids_changed.connect(
        [this](const auto& control_ids) {
          divertable_control_ids_changed(control_ids);
        });

    session_controller_->start();
  }

  pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread_;
  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  device_identifiers identifiers_;
  std::string log_prefix_;
  std::optional<core_configuration::details::logitech_hidpp> applied_configuration_;
  std::vector<uint16_t> applied_control_ids_;
  std::shared_ptr<logitech_hidpp_session_controller> session_controller_;

  // Construct after potentially throwing members; destruction requires detach.
  pqrs::dispatcher::extra::debounced_task update_task_{*this};
};
} // namespace krbn::core_service::daemon::device_grabber_details
