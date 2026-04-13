#pragma once

#include "app_icon.hpp"
#include "codesign_manager.hpp"
#include "constants.hpp"
#include "core_service/daemon/components_manager.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "services_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>
#include <mach/mach.h>
#include <nlohmann/json.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/workspace.hpp>
#include <thread>

namespace krbn {
namespace core_service {
namespace core_service_utility {

static constexpr const char* karabiner_core_service_bundle_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Core-Service.app";

class permission_check_result final {
public:
  permission_check_result(void)
      : input_monitoring_granted_(false),
        accessibility_process_trusted_(false) {
  }

  bool get_input_monitoring_granted(void) const {
    return input_monitoring_granted_;
  }

  void set_input_monitoring_granted(bool value) {
    input_monitoring_granted_ = value;
  }

  bool get_accessibility_process_trusted(void) const {
    return accessibility_process_trusted_;
  }

  void set_accessibility_process_trusted(bool value) {
    accessibility_process_trusted_ = value;
  }

private:
  bool input_monitoring_granted_;
  bool accessibility_process_trusted_;
};

inline void to_json(nlohmann::json& json, const permission_check_result& value) {
  json = nlohmann::json::object({
      {"input_monitoring_granted", value.get_input_monitoring_granted()},
      {"accessibility_process_trusted", value.get_accessibility_process_trusted()},
  });
}

inline void from_json(const nlohmann::json& json, permission_check_result& value) {
  value.set_input_monitoring_granted(json.at("input_monitoring_granted").get<bool>());
  value.set_accessibility_process_trusted(json.at("accessibility_process_trusted").get<bool>());
}

inline permission_check_result make_permission_check_result(void) {
  permission_check_result result;
  result.set_input_monitoring_granted(IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) == kIOHIDAccessTypeGranted);
  result.set_accessibility_process_trusted(pqrs::osx::accessibility::is_process_trusted());
  return result;
}

inline std::filesystem::path get_permission_check_result_json_file_path(void) {
  return constants::get_user_tmp_directory() / "core-service-permission-check-result.json";
}

inline std::optional<permission_check_result> run_permission_check(void) {
  filesystem_utility::mkdir_user_directories();

  auto result_json_file_path = get_permission_check_result_json_file_path();
  std::error_code ec;
  std::filesystem::remove(result_json_file_path, ec);

  pqrs::osx::workspace::open_application_by_bundle_path(
      karabiner_core_service_bundle_path,
      pqrs::osx::workspace::open_configuration{
          .activates = false,
          .adds_to_recent_items = false,
          .arguments = {
              "permission-check",
              result_json_file_path.string(),
          }});

  for (int i = 0; i < 50; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!pqrs::filesystem::exists(result_json_file_path)) {
      continue;
    }

    try {
      std::ifstream input(result_json_file_path);
      auto json = nlohmann::json::parse(input);
      return json.get<permission_check_result>();
    } catch (const std::exception& e) {
      logger::get_logger()->error("failed to read permission check result: {0}", e.what());
      break;
    }
  }

  return std::nullopt;
}

} // namespace core_service_utility
} // namespace core_service
} // namespace krbn
