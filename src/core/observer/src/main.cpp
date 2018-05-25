#include "constants.hpp"
#include "device_observer.hpp"
#include "filesystem.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include "spdlog_utility.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_observer requires root privilege." << std::endl;
    exit(1);
  }

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  krbn::thread_utility::register_main_thread();

  // ----------------------------------------
  {
    auto log_directory = "/var/log/karabiner";
    mkdir(log_directory, 0755);
    if (krbn::filesystem::is_directory(log_directory)) {
      spdlog::set_async_mode(4096);
      auto l = spdlog::rotating_logger_mt("observer", "/var/log/karabiner/observer.log", 256 * 1024, 3);
      l->flush_on(spdlog::level::info);
      l->set_pattern(krbn::spdlog_utility::get_pattern());
      krbn::logger::set_logger(l);
    }
  }

  krbn::logger::get_logger().info("version {0}", karabiner_version);

  {
    std::string pid_file_path = std::string(krbn::constants::get_tmp_directory()) + "/karabiner_observer.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      std::string message("Exit since another process is running.");
      krbn::logger::get_logger().info(message);
      std::cerr << message << std::endl;
      return 0;
    }
  }

  auto version_monitor_ptr = std::make_unique<krbn::version_monitor>([] {
    exit(0);
  });

  auto device_observer_ptr = std::make_unique<krbn::device_observer>();

  CFRunLoopRun();

  device_observer_ptr = nullptr;
  version_monitor_ptr = nullptr;

  return 0;
}
