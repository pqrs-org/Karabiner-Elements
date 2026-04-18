#pragma once

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "types/core_service_permission_check_result.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/filesystem.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/workspace.hpp>
#include <thread>

namespace krbn {
namespace core_service {
namespace core_service_utility {

static constexpr const char* karabiner_core_service_bundle_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Core-Service.app";

inline core_service_permission_check_result make_permission_check_result_for_current_process() {
  core_service_permission_check_result result;
  result.set_input_monitoring_granted(IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) == kIOHIDAccessTypeGranted);
  result.set_accessibility_process_trusted(pqrs::osx::accessibility::is_process_trusted());
  return result;
}

inline void prompt_permissions() {
  IOHIDCheckAccess(kIOHIDRequestTypeListenEvent);
  pqrs::osx::accessibility::is_process_trusted_with_prompt();
}

inline std::optional<core_service_permission_check_result> run_permission_check_for_new_core_service_instance() {
  filesystem_utility::mkdir_user_directories();

  auto result_json_file_path =
      constants::get_user_tmp_directory() / "core-service-permission-check-result.json";
  std::error_code ec;
  std::filesystem::remove(result_json_file_path, ec);

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

    if (!pqrs::filesystem::exists(result_json_file_path)) {
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

} // namespace core_service_utility
} // namespace core_service
} // namespace krbn
