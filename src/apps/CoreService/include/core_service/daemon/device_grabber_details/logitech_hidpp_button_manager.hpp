#pragma once

#include "core_configuration/details/profile/logitech_hidpp.hpp"
#include "hid_device_events_monitor.hpp"
#include "hid_report_only_events/logitech/hidpp/button.hpp"
#include "hid_report_only_events/logitech/hidpp/device_support.hpp"
#include "logitech_hidpp_session_controller.hpp"
#include <nod/nod.hpp>

namespace krbn::core_service::daemon::device_grabber_details {
class logitech_hidpp_button_manager final {
public:
  nod::signal<void(const std::vector<uint16_t>&)> divertable_control_ids_changed;
  nod::signal<void(uint16_t, bool)> button_event_generated;
  nod::signal<void(uint16_t)> control_abandoned;

  logitech_hidpp_button_manager(const logitech_hidpp_button_manager&) = delete;

  logitech_hidpp_button_manager(pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                                IOHIDDeviceRef device,
                                const device_identifiers& identifiers,
                                const std::string& log_prefix)
      : run_loop_thread_(run_loop_thread),
        device_(device),
        log_prefix_(log_prefix) {
    std::vector<uint8_t> report_descriptor;
    if (!logitech::hidpp::device_support::find_unsupported_device_reason(identifiers)) {
      report_descriptor = hid_device_events_monitor::find_report_descriptor(device);
    }

    device_support_ = logitech::hidpp::device_support(identifiers,
                                                      report_descriptor);
    if (!device_support_.get_unsupported_reason()) {
      report_handler_ = std::make_shared<hid_report_only_events::logitech::hidpp::button::handler>();
    }
  }

  ~logitech_hidpp_button_manager() {
    if (report_handler_) {
      report_handler_->clear_input_report_handler();
    }
    session_controller_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<hid_report_only_events::logitech::hidpp::button::handler> get_report_handler() const {
    return report_handler_;
  }

  // Applies the desired setting. A session without a selected control is retained for discovery.
  // This method should be called in the shared dispatcher thread while the device is open.
  void update(const std::optional<core_configuration::details::logitech_hidpp>& desired,
              bool device_is_seized) {
    // The output mapping is handled by the Simple Modifications manipulator manager.
    // Keep the HID++ session when only a mapping changes, so the same controls are not
    // restored and diverted again.
    if (session_controller_) {
      const auto desired_control_ids = desired ? desired->get_enabled_control_ids() : std::vector<uint16_t>{};
      const auto current_control_ids = applied_configuration_ ? applied_configuration_->get_enabled_control_ids() : std::vector<uint16_t>{};
      if (desired_control_ids == current_control_ids) {
        applied_configuration_ = desired;
        return;
      }
    }

    stop(device_is_seized);

    if (!report_handler_) {
      return;
    }

    applied_configuration_ = desired;

    session_controller_ = std::make_shared<logitech_hidpp_session_controller>(run_loop_thread_,
                                                                              *device_,
                                                                              desired ? desired->get_enabled_control_ids() : std::vector<uint16_t>{},
                                                                              log_prefix_);
    session_controller_->divertable_control_ids_changed.connect(
        [this](const auto& control_ids) {
          divertable_control_ids_changed(control_ids);
        });

    report_handler_->set_input_report_handler(
        [weak_controller = std::weak_ptr(session_controller_)](auto report) {
          if (auto controller = weak_controller.lock()) {
            return controller->handle_input_report(report);
          }
          return std::vector<logitech::hidpp::session::result::button_event>{};
        });

    session_controller_->start();
  }

  // Releases held buttons and restores diverted controls if the device can still be written.
  // Otherwise, forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread.
  void stop(bool device_is_seized) {
    if (!device_is_seized) {
      abandon();
      return;
    }

    if (session_controller_) {
      for (const auto& event : session_controller_->stop()) {
        if (applied_configuration_ &&
            applied_configuration_->is_enabled(event.control_id)) {
          button_event_generated(event.control_id,
                                 event.pressed);
        }
      }
      report_handler_->clear_input_report_handler();
      session_controller_ = nullptr;
    }

    applied_configuration_ = std::nullopt;
  }

  // Forgets the session without writing to the device.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon() {
    if (session_controller_) {
      session_controller_->abandon();
      report_handler_->clear_input_report_handler();
      session_controller_ = nullptr;

      if (applied_configuration_) {
        for (auto control_id : applied_configuration_->get_enabled_control_ids()) {
          control_abandoned(control_id);
        }
      }
    }

    applied_configuration_ = std::nullopt;
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

  [[nodiscard]] bool configuration_is_applied(const std::optional<core_configuration::details::logitech_hidpp>& configuration) const {
    return configuration == applied_configuration_;
  }

private:
  pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread_;
  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  std::string log_prefix_;
  logitech::hidpp::device_support device_support_;
  std::shared_ptr<hid_report_only_events::logitech::hidpp::button::handler> report_handler_;
  std::optional<core_configuration::details::logitech_hidpp> applied_configuration_;
  std::shared_ptr<logitech_hidpp_session_controller> session_controller_;
};
} // namespace krbn::core_service::daemon::device_grabber_details
