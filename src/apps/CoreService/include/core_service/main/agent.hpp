#pragma once

#include "core_service/agent/components_manager.hpp"
#include "core_service/core_service_utility.hpp"
#include "environment_variable_utility.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_lifecycle_manager.hpp"
#include "services_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/application.hpp>

namespace krbn::core_service::main {
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

        if (!filesystem_utility::rename(temporary_result_json_file_path,
                                        result_json_file_path)) {
          return 1;
        }

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
  // If the Karabiner-Console-User-Server LaunchAgent plist is renamed in an update,
  // the agent will not start automatically until it is registered again.
  //

  services_utility::register_core_agents();

  //
  // The agent opens karabiner.json to trigger the disk-access permission prompt,
  // in case ~/.config/karabiner is a symlink and karabiner.json lives under Documents or similar.
  //

  std::ifstream input(constants::get_user_core_configuration_file_path());

  //
  // Run process_lifecycle_manager
  //

  process_lifecycle_manager::initialize_shared_instance(
      process_lifecycle_manager::configuration{
          .components_manager_maker =
              [] {
                return std::make_unique<core_service::agent::components_manager>();
              },
          .termination_completion_handler =
              [] {
                dispatch_async(dispatch_get_main_queue(), ^{
                  pqrs::osx::application::stop();
                });
              },
      });

  // This is needed because this agent uses pqrs::osx::application::run.
  // AppKit termination requests should go through process_lifecycle_manager so
  // components and the power management monitor are destroyed before the AppKit run loop stops.
  pqrs::osx::application::set_should_terminate_callback([] {
    if (process_lifecycle_manager::async_request_termination()) {
      return pqrs::osx::application::terminate_reply::cancel;
    }

    return pqrs::osx::application::terminate_reply::now;
  });

  process_lifecycle_manager::async_start();

  // Use the AppKit application run loop because the agent uses
  // pqrs::osx::accessibility::monitor and other AppKit/main-thread APIs.
  pqrs::osx::application::run();

  process_lifecycle_manager::terminate_shared_instance();

  return 0;
}
} // namespace krbn::core_service::main
