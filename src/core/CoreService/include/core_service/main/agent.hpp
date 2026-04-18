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
        std::cerr << "missing result path" << std::endl;
        return 1;
      }

      auto result = core_service::core_service_utility::make_permission_check_result_for_current_process();
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
        std::cerr << e.what() << std::endl;
        return 1;
      }

    } else {
      std::cout << "unsupported argument: " << args[i] << std::endl;
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

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  core_service::agent::components_manager* components_manager = nullptr;

  if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([] {
      dispatch_async(dispatch_get_main_queue(), ^{
        pqrs::osx::application::stop();
      });
    });
  }

  components_manager = new core_service::agent::components_manager();
  components_manager->async_start();

  pqrs::osx::application::run();

  {
    // Mark as main queue to avoid a deadlock in `pqrs::gcd::dispatch_sync_on_main_queue` in destructor.
    pqrs::gcd::scoped_running_on_main_queue_marker marker;

    delete components_manager;
    components_manager = nullptr;
  }

  components_manager_killer::terminate_shared_components_manager_killer();

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
