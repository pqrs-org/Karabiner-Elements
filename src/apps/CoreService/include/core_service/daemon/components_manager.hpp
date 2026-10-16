#pragma once

// `krbn::core_service::daemon::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "filesystem_utility.hpp"
#include "hid_event_system_monitor.hpp"
#include "logger.hpp"
#include "process_lifecycle_manager.hpp"
#include "receiver.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/session.hpp>

namespace krbn::core_service::daemon {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  components_manager(const components_manager&) = delete;

  components_manager(std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager)
      : weak_core_service_daemon_state_manager_(weak_core_service_daemon_state_manager) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
          //
          // hid_event_system_monitor_
          //

          hid_event_system_monitor_ = std::make_unique<hid_event_system_monitor>();
        });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      stop_receivers();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      start_receivers();
      enqueue_ensure_base_directories();
    });
  }

private:
  void start_receivers() {
    current_console_user_id_ = std::nullopt;

    start_receiver(std::nullopt);
  }

  void stop_receivers() {
    receiver_ = nullptr;
    current_console_user_id_ = std::nullopt;
  }

  void start_receiver(std::optional<uid_t> uid) {
    current_console_user_id_ = uid;

    // receiver_

    receiver_ = nullptr;
    receiver_ = std::make_unique<receiver>(uid,
                                           weak_core_service_daemon_state_manager_);

    receiver_->console_user_id_changed.connect([this](auto uid) {
      if (uid) {
        logger::get_logger()->info("current_console_user_id: {0}",
                                   *uid);
      } else {
        logger::get_logger()->info("current_console_user_id: none");
      }

      // Defer destruction of the current receiver until signal delivery has
      // returned to its request handler.
      enqueue_to_dispatcher([this, uid] {
        start_receiver(uid);
      });
    });

    receiver_->listener_recovery_required.connect([this](const auto& request) {
      handle_listener_recovery_request(request);
    });
  }

  void handle_listener_recovery_request(const pqrs::osx::xpc::listener_recovery_request& request) {
    logger::get_logger()->error("core_service daemon XPC listener recovery is required: {0}",
                                request.error.message());

    switch (request.recommended_action) {
      case pqrs::osx::xpc::listener_recovery_action::recreate_listener:
        // Defer destruction of the failed listener until signal delivery has
        // returned, then create a fresh listener in this process.
        enqueue_to_dispatcher(
            [this] {
              start_receiver(current_console_user_id_);
            },
            when_now() + std::chrono::seconds(1));
        break;

      case pqrs::osx::xpc::listener_recovery_action::restart_process:
        // A named listener that has lost its Mach service registration needs a
        // fresh process. Let launchd restart this KeepAlive job.
        process_lifecycle_manager::async_request_termination();
        break;

      case pqrs::osx::xpc::listener_recovery_action::correct_configuration:
        // The options are compiled into the application and cannot be corrected
        // at runtime. Stop this unusable process rather than retrying forever in
        // process with the same configuration.
        process_lifecycle_manager::async_request_termination();
        break;
    }
  }

  void enqueue_ensure_base_directories() {
    enqueue_to_dispatcher(
        [this] {
          if (base_directories_missing()) {
            filesystem_utility::prepare_system_directories(current_console_user_id_);
          }

          enqueue_ensure_base_directories();
        },
        when_now() + std::chrono::seconds(3));
  }

  [[nodiscard]] bool base_directories_missing() const {
    if (!filesystem_utility::exists(constants::get_tmp_directory()) ||
        !filesystem_utility::exists(constants::get_rootonly_directory())) {
      return true;
    }

    if (current_console_user_id_) {
      if (!filesystem_utility::exists(constants::get_system_user_directory()) ||
          !filesystem_utility::exists(constants::get_system_user_directory(*current_console_user_id_))) {
        return true;
      }
    }

    return false;
  }

  std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager_;
  std::optional<uid_t> current_console_user_id_;

  std::unique_ptr<hid_event_system_monitor> hid_event_system_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn::core_service::daemon
