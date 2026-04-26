#pragma once

#include "core_service/agent/components_manager.hpp"
#include "core_service/core_service_utility.hpp"
#include "environment_variable_utility.hpp"
#include "logger.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/application.hpp>

namespace krbn {
namespace core_service {
namespace main {
int agent(std::vector<std::string> args) {
  auto log_cli_error = [](const std::string& message) {
    if (!constants::get_user_log_directory().empty()) {
      logger::set_async_rotating_logger("core_service (cli)",
                                        constants::get_user_log_directory() / "core_service_cli.log",
                                        pqrs::spdlog::filesystem::log_directory_perms_0700);
    }
    logger::get_logger()->error(message);
    std::cerr << message << std::endl;
  };

  //
  // Call NSApplication.shared.finishLaunching() in order to avoid the following error
  // when the app is launched by the open command or similar methods.
  // This is especially problematic when it is executed with the permission-check argument.
  //
  // _LSOpenURLsWithCompletionHandler() failed for the application Karabiner-Core-Service.app with error -1712.
  //

  pqrs::osx::application::enable_stop_on_terminate();
  pqrs::osx::application::finish_launching();

  //
  // Process arguments
  //

  for (std::size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "permission-check") {
      if (i + 1 >= args.size()) {
        log_cli_error("missing result path");
        return 1;
      }

      auto result = core_service::core_service_utility::make_current_process_permission_check_result();
      auto result_json_file_path = std::filesystem::path(args[i + 1]);
      auto temporary_result_json_file_path = result_json_file_path;
      temporary_result_json_file_path += ".tmp";

      try {
        std::ofstream output(temporary_result_json_file_path);
        output << nlohmann::json(result).dump();
        output.close();

        std::filesystem::rename(temporary_result_json_file_path,
                                result_json_file_path);

        return 0;
      } catch (const std::exception& e) {
        log_cli_error(e.what());
        return 1;
      }

    } else {
      log_cli_error("unsupported argument: " + args[i]);
      return 1;
    }
  }

  //
  // Load custom environment variables
  //

  auto environment_variables = environment_variable_utility::load_custom_environment_variables();

  //
  // Setup logger
  //

  if (!constants::get_user_log_directory().empty()) {
    logger::set_async_rotating_logger("core_service (agent)",
                                      constants::get_user_log_directory() / "core_service.log",
                                      pqrs::spdlog::filesystem::log_directory_perms_0700);
  }

  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Log custom environment variables
  //

  environment_variable_utility::log(environment_variables);

  //
  // Get codesign
  //

  get_shared_codesign_manager()->log();

  //
  // The agent opens karabiner.json to trigger the disk-access permission prompt,
  // in case ~/.config/karabiner is a symlink and karabiner.json lives under Documents or similar.
  //

  std::ifstream input(constants::get_user_core_configuration_file_path());

  //
  // Run components_manager
  //

  components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use a raw pointer to control the destruction timing from `kill_called`.
  core_service::agent::components_manager* components_manager = nullptr;

  if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([&components_manager] {
      // Destroy `components_manager` before stopping the main run loop so that
      // cleanup code that uses `gcd::dispatch_sync_on_main_queue` can still run
      // while the main thread is servicing the run loop.
      // `kill_called` is invoked on the shared dispatcher thread, so we destroy
      // `components_manager` there before calling `CFRunLoopStop`.
      if (components_manager) {
        delete components_manager;
        components_manager = nullptr;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        pqrs::osx::application::stop();
      });
    });
  }

  components_manager = new core_service::agent::components_manager();
  components_manager->async_start();

  pqrs::osx::application::run();

  //
  // Cleanup
  //

  components_manager_killer::terminate_shared_components_manager_killer();

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
