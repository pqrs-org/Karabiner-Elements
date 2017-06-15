#include "connection_manager.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "notification_center.hpp"
#include "process_utility.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"
#include "virtual_hid_device_client.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
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
      auto l = spdlog::rotating_logger_mt("grabber", "/var/log/karabiner/grabber.log", 256 * 1024, 3);
      l->flush_on(spdlog::level::info);
      l->set_pattern(krbn::spdlog_utility::get_pattern());
      krbn::logger::set_logger(l);
    }
  }

  krbn::logger::get_logger().info("version {0}", karabiner_version);

  {
    std::string pid_file_path = std::string(krbn::constants::get_tmp_directory()) + "/karabiner_grabber.pid";
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

  // load kexts
  system("/sbin/kextload /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext");

  // make socket directory.
  mkdir(krbn::constants::get_tmp_directory(), 0755);
  chown(krbn::constants::get_tmp_directory(), 0, 0);
  chmod(krbn::constants::get_tmp_directory(), 0755);

  unlink(krbn::constants::get_grabber_socket_file_path());

  auto virtual_hid_device_client_ptr = std::make_unique<krbn::virtual_hid_device_client>();
  virtual_hid_device_client_ptr->connect();
  auto device_grabber_ptr = std::make_unique<krbn::device_grabber>(*virtual_hid_device_client_ptr);
  krbn::connection_manager connection_manager(*version_monitor_ptr, *device_grabber_ptr);

  krbn::notification_center::post_distributed_notification_to_all_sessions(krbn::constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  device_grabber_ptr = nullptr;
  version_monitor_ptr = nullptr;

  return 0;
}
