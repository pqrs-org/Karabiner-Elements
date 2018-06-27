#include "apple_notification_center.hpp"
#include "connection_manager.hpp"
#include "constants.hpp"
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

namespace krbn {
class karabiner_observer final {
public:
  karabiner_observer(void) {
    {
      auto log_directory = "/var/log/karabiner";
      mkdir(log_directory, 0755);
      if (filesystem::is_directory(log_directory)) {
        spdlog::set_async_mode(4096);
        auto l = spdlog::rotating_logger_mt("observer", "/var/log/karabiner/observer.log", 256 * 1024, 3);
        l->flush_on(spdlog::level::info);
        l->set_pattern(spdlog_utility::get_pattern());
        logger::set_logger(l);
      }
    }

    logger::get_logger().info("version {0}", karabiner_version);

    {
      std::string pid_file_path = std::string(constants::get_tmp_directory()) + "/karabiner_observer.pid";
      if (!process_utility::lock_single_application(pid_file_path)) {
        std::string message("Exit since another process is running.");
        logger::get_logger().info(message);
        std::cerr << message << std::endl;
        exit(0);
      }
    }

    version_monitor_ = std::make_unique<version_monitor>([] {
      exit(0);
    });

    refresh_connection_manager();

    apple_notification_center::observe_distributed_notification(this,
                                                                static_grabber_is_launched_callback,
                                                                constants::get_distributed_notification_grabber_is_launched());
  }

  ~karabiner_observer(void) {
    connection_manager_ = nullptr;
    version_monitor_ = nullptr;
  }

  void refresh_connection_manager(void) {
    connection_manager_ = nullptr;
    connection_manager_ = std::make_unique<connection_manager>(*version_monitor_);
  }

private:
  static void static_grabber_is_launched_callback(CFNotificationCenterRef center,
                                                  void* observer,
                                                  CFStringRef notification_name,
                                                  const void* observed_object,
                                                  CFDictionaryRef user_info) {
    auto self = static_cast<karabiner_observer*>(observer);
    self->grabber_is_launched_callback();
  }

  void grabber_is_launched_callback(void) {
    logger::get_logger().info("grabber_is_launched_callback");
    refresh_connection_manager();
  }

  std::unique_ptr<version_monitor> version_monitor_;
  std::unique_ptr<connection_manager> connection_manager_;
};
} // namespace krbn

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_observer requires root privilege." << std::endl;
    exit(1);
  }

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  krbn::thread_utility::register_main_thread();

  krbn::karabiner_observer karabiner_observer;

  CFRunLoopRun();

  return 0;
}
