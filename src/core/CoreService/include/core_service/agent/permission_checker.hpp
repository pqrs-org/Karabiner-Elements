#pragma once

#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include "types/core_service_permission_check_result.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/workspace.hpp>
#include <thread>

namespace krbn::core_service::agent {

class permission_checker final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(const core_service_permission_check_result&)> permission_check_result_changed;

  permission_checker(const permission_checker&) = delete;

  permission_checker()
      : dispatcher_client(),
        check_permissions_task_(*this) {
  }

  ~permission_checker() override {
    detach_from_dispatcher();
  }

  void async_set_on_console(std::optional<bool> value) {
    enqueue_to_dispatcher([this, value] {
      on_console_ = value;
      check_permissions();
    });
  }

  void enqueue_check_permissions(std::chrono::milliseconds delay = std::chrono::milliseconds(0)) {
    check_permissions_task_.debounce_after(
        [this] {
          check_permissions();
        },
        delay);
  }

private:
  void check_permissions() {
    if (on_console_ != std::optional<bool>(true)) {
      return;
    }

    // `make_bundle_permission_check_result` blocks while waiting for the permission-check result file.
    // This is acceptable here because this path is only relevant while the required permissions are not granted,
    // and during that period the agent has little else to do besides prompting for and re-checking permissions.
    auto result = make_bundle_permission_check_result();
    if (!result) {
      enqueue_check_permissions(std::chrono::seconds(1));
      return;
    }

    // On macOS 26, Accessibility permission may also cover Input Monitoring permission.
    // (Note that on macOS 14, Input Monitoring permission is still required separately even if Accessibility is granted.)
    // Therefore, request Accessibility permission first, and only request Input Monitoring permission
    // if it is still needed after Accessibility has already been granted.
    if (!result->get_accessibility_process_trusted()) {
      prompt_accessibility_permission_once();
    } else {
      if (!result->get_iohid_listen_event_allowed()) {
        prompt_input_monitoring_permission_once();
      }
    }

    // If permissions were revoked while the agent kept running, the cached bundle permission state can become stale.
    // In that case, terminate the agent and let launchd restart the agent.
    // The restarted process can re-enter the normal prompt-and-poll flow from a clean state.
    if (last_bundle_permission_check_result_ &&
        last_bundle_permission_check_result_->required_permissions_granted() &&
        !result->required_permissions_granted()) {
      logger::get_logger()->info("Required permissions were revoked. Terminating the agent.");

      last_bundle_permission_check_result_ = *result;

      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
      return;
    }

    if (!result->required_permissions_granted()) {
      restart_required_after_permissions_granted_ = true;
      enqueue_check_permissions(std::chrono::seconds(1));
    }

    last_bundle_permission_check_result_ = *result;
    permission_check_result_changed(*result);

    if (!result->required_permissions_granted()) {
      return;
    }

    if (restart_required_after_permissions_granted_) {
      logger::get_logger()->info("The required permissions are granted. Restarting core daemons and terminating the agent.");

      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    }
  }

  static constexpr const char* karabiner_core_service_bundle_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Core-Service.app";

  static std::optional<core_service_permission_check_result> make_bundle_permission_check_result() {
    filesystem_utility::prepare_user_directories();

    auto result_json_file_path =
        constants::get_user_tmp_directory() / "core-service-permission-check-result.json";
    filesystem_utility::remove(result_json_file_path);

    pqrs::osx::workspace::open_application_by_bundle_path(
        karabiner_core_service_bundle_path,
        pqrs::osx::workspace::open_configuration{
            .activates = false,
            .adds_to_recent_items = false,
            .creates_new_application_instance = true,
            .arguments = {
                "permission-check",
                result_json_file_path.string(),
            }});

    for (int i = 0; i < 50; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      if (!filesystem_utility::exists(result_json_file_path)) {
        continue;
      }

      try {
        std::ifstream input(result_json_file_path);
        auto json = nlohmann::json::parse(input);
        return json.get<core_service_permission_check_result>();
      } catch (const std::exception& e) {
        logger::get_logger()->error("failed to read permission check result: {0}", e.what());
        break;
      }
    }

    return std::nullopt;
  }

  static void prompt_accessibility_permission_once() {
    static bool prompted = false;

    if (prompted) {
      return;
    }

    prompted = true;
    pqrs::osx::accessibility::is_process_trusted_with_prompt();
  }

  static void prompt_input_monitoring_permission_once() {
    static bool prompted = false;

    if (prompted) {
      return;
    }

    prompted = true;
    IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
  }

  std::optional<bool> on_console_;
  std::optional<core_service_permission_check_result> last_bundle_permission_check_result_;
  bool restart_required_after_permissions_granted_ = false;
  pqrs::dispatcher::extra::debounced_task check_permissions_task_;
};

} // namespace krbn::core_service::agent
