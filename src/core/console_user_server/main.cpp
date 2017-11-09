#include "connection_manager.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "grabber_alerts_monitor.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "migration.hpp"
#include "process_utility.hpp"
#include "spdlog_utility.hpp"
#include "update_utility.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  krbn::thread_utility::register_main_thread();

  {
    auto log_directory = krbn::constants::get_user_log_directory();
    if (!log_directory.empty()) {
      krbn::filesystem::create_directory_with_intermediate_directories(log_directory, 0700);

      if (krbn::filesystem::is_directory(log_directory)) {
        std::string log_file_path = log_directory + "/console_user_server.log";
        auto l = spdlog::rotating_logger_mt("console_user_server", log_file_path.c_str(), 256 * 1024, 3);
        l->flush_on(spdlog::level::info);
        l->set_pattern(krbn::spdlog_utility::get_pattern());
        krbn::logger::set_logger(l);
      }
    }
  }

  krbn::logger::get_logger().info("version {0}", karabiner_version);

  if (!krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_console_user_server.pid")) {
    std::string message("Exit since another process is running.");
    krbn::logger::get_logger().info(message);
    std::cerr << message << std::endl;
    return 0;
  }

  auto version_monitor_ptr = std::make_unique<krbn::version_monitor>([] {
    exit(0);
  });

  auto grabber_alerts_monitor_ptr = std::make_unique<krbn::grabber_alerts_monitor>([] {
    krbn::logger::get_logger().info("karabiner_grabber_alerts.json is updated.");
    krbn::application_launcher::launch_preferences();
  });

  krbn::migration::migrate_v1();

  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_configuration_directory(), 0700);
  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_complex_modifications_assets_directory(), 0700);

  krbn::connection_manager manager(*version_monitor_ptr);

  krbn::update_utility::check_for_updates_on_startup();

  CFRunLoopRun();

  version_monitor_ptr = nullptr;

  return 0;
}
