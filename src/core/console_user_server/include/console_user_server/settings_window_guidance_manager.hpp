#pragma once

#include "application_launcher.hpp"
#include "logger.hpp"
#include "services_utility.hpp"
#include "types/settings_window_guidance_state.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace console_user_server {
class settings_window_guidance_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  using guidance_context_maker = std::function<settings_window_guidance_context()>;
  using launch_settings_handler = std::function<void()>;

  static guidance_context_maker make_default_guidance_context_maker(void) {
    return [] {
      settings_window_guidance_context c;

      // Note:
      // services_utility::*_enabled and services_utility::*_running may take time because they trigger process launches.
      c.set_core_daemons_enabled(services_utility::core_daemons_enabled());
      c.set_core_agents_enabled(services_utility::core_agents_enabled());
      c.set_core_daemons_running(services_utility::core_daemons_running());
      c.set_core_agents_running(services_utility::core_agents_running());

      return c;
    };
  }

  static launch_settings_handler make_default_launch_settings_handler(void) {
    return [] {
      application_launcher::launch_settings_without_activation();
    };
  }

  settings_window_guidance_manager(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                                   guidance_context_maker guidance_context_maker,
                                   launch_settings_handler launch_settings_handler = make_default_launch_settings_handler())
      : dispatcher_client(weak_dispatcher),
        guidance_context_maker_(std::move(guidance_context_maker)),
        launch_settings_handler_(std::move(launch_settings_handler)),
        timer_(*this) {
  }

  ~settings_window_guidance_manager() {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start() {
    timer_.start(
        [this] {
          auto c = guidance_context_maker_();

          {
            std::lock_guard<std::mutex> lock(mutex_);

            update_guidance_context(c);
            update_current_guidance();
          }

          launch_settings_if_needed();
        },
        std::chrono::seconds(3));
  }

  void async_update_core_service_daemon_state(const core_service_daemon_state& state) {
    enqueue_to_dispatcher([this, state] {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        update_core_service_daemon_state_conditions(state);
        update_current_guidance();
      }

      launch_settings_if_needed();
    });
  }

  settings_window_guidance_state get_guidance_state() const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto state = settings_window_guidance_state();
    state.set_current_setup(current_setup_);
    state.set_current_alert(current_alert_);
    state.set_guidance_context(guidance_context_);
    state.set_core_service_daemon_state(core_service_deamon_state_);

    return state;
  }

private:
  // This method is executed in the dedicated dispatcher thread.
  void update_current_guidance() {
    auto new_current_setup = make_current_setup();
    // Do not show alerts until setup is complete.
    // If alerts take priority in the UI, the required setup may never be completed.
    auto new_current_alert = new_current_setup == settings_window_guidance_setup::none
                                 ? make_current_alert()
                                 : settings_window_guidance_alert::none;
    auto previous_setup = settings_window_guidance_setup::none;
    auto previous_alert = settings_window_guidance_alert::none;
    auto setup_changed = false;
    auto alert_changed = false;

    setup_changed = current_setup_ != new_current_setup;
    alert_changed = current_alert_ != new_current_alert;
    if (!setup_changed &&
        !alert_changed) {
      return;
    }

    previous_setup = current_setup_;
    previous_alert = current_alert_;
    current_setup_ = new_current_setup;
    current_alert_ = new_current_alert;

    if (setup_changed) {
      logger::get_logger()->info("settings_window_guidance_setup changed: {0} -> {1}",
                                 nlohmann::json(previous_setup).get<std::string>(),
                                 nlohmann::json(new_current_setup).get<std::string>());
    }
    if (alert_changed) {
      logger::get_logger()->info("settings_window_guidance_alert changed: {0} -> {1}",
                                 nlohmann::json(previous_alert).get<std::string>(),
                                 nlohmann::json(new_current_alert).get<std::string>());
    }

    if (new_current_setup != settings_window_guidance_setup::none ||
        new_current_alert != settings_window_guidance_alert::none) {
      needs_to_launch_settings_ = true;
    }
  }

  // This method is executed in the dedicated dispatcher thread.
  void update_guidance_context(const settings_window_guidance_context& c) {
    auto previous_services_enabled = guidance_context_.services_enabled();
    auto new_services_enabled = c.services_enabled();

    if (previous_services_enabled != new_services_enabled &&
        new_services_enabled == std::optional<bool>(true)) {
      services_enabled_at_ = when_now();
    }

    guidance_context_ = c;
  }

  // This method is executed in the dedicated dispatcher thread.
  void update_core_service_daemon_state_conditions(const core_service_daemon_state& state) {
    core_service_deamon_state_ = state;

    //
    // karabiner_json_parse_error_message_
    //

    if (karabiner_json_parse_error_message_ != state.get_karabiner_json_parse_error_message()) {
      karabiner_json_parse_error_message_ = state.get_karabiner_json_parse_error_message();

      if (!karabiner_json_parse_error_message_.empty()) {
        karabiner_json_parse_error_started_at_ = when_now();
      }
    }

    //
    // virtual_hid_device_service_client_connected_
    //

    if (virtual_hid_device_service_client_connected_ != state.get_virtual_hid_device_service_client_connected()) {
      virtual_hid_device_service_client_connected_ = state.get_virtual_hid_device_service_client_connected();

      if (virtual_hid_device_service_client_connected_ == std::optional<bool>(false)) {
        virtual_hid_device_service_client_not_connected_started_at_ = when_now();
      }
    }

    //
    // driver_connected_
    //

    if (driver_connected_ != state.get_driver_connected()) {
      driver_connected_ = state.get_driver_connected();

      if (driver_connected_ == std::optional<bool>(false)) {
        driver_not_connected_started_at_ = when_now();
      }
    }
  }

  // This method is executed in the dedicated dispatcher thread.
  settings_window_guidance_setup make_current_setup() const {
    if (guidance_context_.services_enabled() == std::optional<bool>(false)) {
      return settings_window_guidance_setup::services;
    }

    auto iohid_listen_event_allowed = std::optional<bool>();
    auto accessibility_process_trusted = std::optional<bool>();
    if (auto permission_check_result = core_service_deamon_state_.get_bundle_permission_check_result()) {
      iohid_listen_event_allowed = permission_check_result->get_iohid_listen_event_allowed();
      accessibility_process_trusted = permission_check_result->get_accessibility_process_trusted();
    }

    // On macOS 26, Accessibility permission may also cover Input Monitoring permission.
    // (Note that on macOS 14, Input Monitoring permission is still required separately even if Accessibility is granted.)
    // Therefore, since Accessibility permission is requested first,
    // the Accessibility setup is also displayed with higher priority than the Input Monitoring setup.
    if (accessibility_process_trusted == std::optional<bool>(false)) {
      return settings_window_guidance_setup::accessibility;
    }

    if (iohid_listen_event_allowed == std::optional<bool>(false)) {
      return settings_window_guidance_setup::input_monitoring;
    }

    if (core_service_deamon_state_.get_driver_activated() == std::optional<bool>(false)) {
      return settings_window_guidance_setup::driver_extension;
    }

    return settings_window_guidance_setup::none;
  }

  // This method is executed in the dedicated dispatcher thread.
  settings_window_guidance_alert make_current_alert() const {
    //
    // doctor
    //

    if (!karabiner_json_parse_error_message_.empty() &&
        when_now() - karabiner_json_parse_error_started_at_ >= std::chrono::seconds(3)) {
      return settings_window_guidance_alert::doctor;
    }

    //
    // settings
    //

    if (core_service_deamon_state_.get_virtual_hid_keyboard_ready().value_or(false) &&
        core_service_deamon_state_.get_virtual_hid_keyboard_type_not_set().value_or(false)) {
      return settings_window_guidance_alert::settings;
    }

    //
    // services_not_running
    //

    if (guidance_context_.services_enabled() == std::optional<bool>(true) &&
        guidance_context_.services_running() == std::optional<bool>(false) &&
        when_now() - services_enabled_at_ >= std::chrono::seconds(10)) {
      return settings_window_guidance_alert::services_not_running;
    }

    //
    // virtual_hid_device_service_client_not_connected
    //

    if (virtual_hid_device_service_client_connected_ == std::optional<bool>(false) &&
        when_now() - virtual_hid_device_service_client_not_connected_started_at_ >= std::chrono::seconds(10)) {
      return settings_window_guidance_alert::virtual_hid_device_service_client_not_connected;
    }

    //
    // driver_version_mismatched
    //

    if (core_service_deamon_state_.get_driver_version_mismatched() == std::optional<bool>(true)) {
      return settings_window_guidance_alert::driver_version_mismatched;
    }

    //
    // driver_not_connected
    //

    if (driver_connected_ == std::optional<bool>(false) &&
        when_now() - driver_not_connected_started_at_ >= std::chrono::seconds(3)) {
      return settings_window_guidance_alert::driver_not_connected;
    }

    return settings_window_guidance_alert::none;
  }

  // This method is executed in the dedicated dispatcher thread.
  void launch_settings_if_needed() {
    if (needs_to_launch_settings_) {
      needs_to_launch_settings_ = false;

      launch_settings_handler_();
    }
  }

  guidance_context_maker guidance_context_maker_;
  launch_settings_handler launch_settings_handler_;

  pqrs::dispatcher::extra::timer timer_;

  mutable std::mutex mutex_;

  settings_window_guidance_setup current_setup_ = settings_window_guidance_setup::none;
  settings_window_guidance_alert current_alert_ = settings_window_guidance_alert::none;
  settings_window_guidance_context guidance_context_;
  core_service_daemon_state core_service_deamon_state_;

  bool needs_to_launch_settings_ = false;

  // For settings_window_guidance_alert::doctor
  std::string karabiner_json_parse_error_message_;
  pqrs::dispatcher::time_point karabiner_json_parse_error_started_at_ = pqrs::dispatcher::time_point(pqrs::dispatcher::duration::zero());

  // For settings_window_guidance_alert::services_not_running
  pqrs::dispatcher::time_point services_enabled_at_ = pqrs::dispatcher::time_point(pqrs::dispatcher::duration::zero());

  // For settings_window_guidance_alert::virtual_hid_device_service_client_not_connected
  std::optional<bool> virtual_hid_device_service_client_connected_;
  pqrs::dispatcher::time_point virtual_hid_device_service_client_not_connected_started_at_ = pqrs::dispatcher::time_point(pqrs::dispatcher::duration::zero());

  // For settings_window_guidance_alert::driver_not_connected
  std::optional<bool> driver_connected_;
  pqrs::dispatcher::time_point driver_not_connected_started_at_ = pqrs::dispatcher::time_point(pqrs::dispatcher::duration::zero());
};
} // namespace console_user_server
} // namespace krbn
