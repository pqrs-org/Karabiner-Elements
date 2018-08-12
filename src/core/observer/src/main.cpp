#include "components_manager.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include "spdlog_utility.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"
#include "version_monitor_utility.hpp"
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
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
        auto l = spdlog::rotating_logger_mt<spdlog::async_factory>("observer",
                                                                   "/var/log/karabiner/observer.log",
                                                                   256 * 1024,
                                                                   3);
        l->flush_on(spdlog::level::info);
        l->set_pattern(spdlog_utility::get_pattern());
        logger::set_logger(l);
      }
    }

    logger::get_logger().info("version {0}", karabiner_version);

    {
      std::string pid_file_path = std::string(constants::get_pid_directory()) + "/karabiner_observer.pid";
      if (!process_utility::lock_single_application(pid_file_path)) {
        std::string message("Exit since another process is running.");
        logger::get_logger().info(message);
        std::cerr << message << std::endl;
        exit(0);
      }
    }

    components_manager_ = std::make_unique<components_manager>();
  }

  ~karabiner_observer(void) {
    components_manager_ = nullptr;
  }

private:
  std::unique_ptr<components_manager> components_manager_;
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
