#pragma once

// `krbn::core_service::daemon::components_manager` can be used safely in a multi-threaded environment.

#include "components_manager_killer.hpp"
#include "console_user_id_changed_receiver.hpp"
#include "constants.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "filesystem_utility.hpp"
#include "hid_event_system_monitor.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "receiver.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/session.hpp>

namespace krbn::core_service::daemon {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager)
      : dispatcher_client(),
        weak_core_service_daemon_state_manager_(weak_core_service_daemon_state_manager) {
    //
    // version_monitor_
    //

    version_monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    version_monitor_->changed.connect([](auto&& version) {
      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    });

    //
    // console_user_id_changed_receiver_
    //

    console_user_id_changed_receiver_ = std::make_unique<console_user_id_changed_receiver>();

    console_user_id_changed_receiver_->current_console_user_id_changed.connect([this](auto&& uid) {
      if (uid) {
        logger::get_logger()->info("current_console_user_id: {0}", *uid);
      } else {
        logger::get_logger()->info("current_console_user_id: none");
      }

      start_receiver(uid);
    });

    //
    // hid_event_system_monitor_
    //

    hid_event_system_monitor_ = std::make_unique<hid_event_system_monitor>();
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      receiver_ = nullptr;
      console_user_id_changed_receiver_ = nullptr;
      version_monitor_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();

      start_receiver(std::nullopt);
      enqueue_ensure_base_directories();

      console_user_id_changed_receiver_->async_start();
    });
  }

private:
  void start_receiver(std::optional<uid_t> uid) {
    current_console_user_id_ = uid;

    version_monitor_->async_manual_check();

    // receiver_

    receiver_ = nullptr;
    receiver_ = std::make_unique<receiver>(uid,
                                           weak_core_service_daemon_state_manager_);
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

  std::unique_ptr<version_monitor> version_monitor_;
  std::unique_ptr<console_user_id_changed_receiver> console_user_id_changed_receiver_;
  std::unique_ptr<hid_event_system_monitor> hid_event_system_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn::core_service::daemon
