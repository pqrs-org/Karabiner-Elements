#include "apple_notification_center.hpp"
#include "components_manager.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "grabber_alerts_manager.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <pqrs/filesystem.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
    exit(1);
  }

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  // Setup logger

  krbn::logger::set_async_rotating_logger("grabber",
                                          "/var/log/karabiner/grabber.log",
                                          0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  // Check another process

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_grabber.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      std::string message("Exit since another process is running.");
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 0;
    }
  }

  // Load kexts

  auto grabber_alerts_manager = std::make_unique<krbn::grabber_alerts_manager>(
      krbn::constants::get_grabber_alerts_json_file_path());

  while (true) {
    std::stringstream ss;
    ss << "/sbin/kextload '"
       << "/Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice/Extensions/"
       << pqrs::karabiner_virtual_hid_device::get_kernel_extension_name()
       << "'";
    krbn::logger::get_logger()->info(ss.str());
    int exit_status = system(ss.str().c_str());
    exit_status >>= 8;
    krbn::logger::get_logger()->info("kextload exit status: {0}", exit_status);

    if (exit_status == 0) {
      break;
    }

    if (exit_status == 27) {
      // kextload is blocked by macOS.
      // https://developer.apple.com/library/content/technotes/tn2459/_index.html
      grabber_alerts_manager->set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, true);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
  }

  grabber_alerts_manager->set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, false);
  grabber_alerts_manager = nullptr;

  // Make socket directory.

  mkdir(krbn::constants::get_tmp_directory(), 0755);
  chown(krbn::constants::get_tmp_directory(), 0, 0);
  chmod(krbn::constants::get_tmp_directory(), 0755);

  unlink(krbn::constants::get_grabber_socket_file_path());

  {
    // Make console_user_server_socket_directory

    std::stringstream ss;
    ss << "rm -r '" << krbn::constants::get_console_user_server_socket_directory() << "'";
    system(ss.str().c_str());

    mkdir(krbn::constants::get_console_user_server_socket_directory(), 0755);
    chown(krbn::constants::get_console_user_server_socket_directory(), 0, 0);
    chmod(krbn::constants::get_console_user_server_socket_directory(), 0755);
  }

  // Run components_manager

  std::shared_ptr<krbn::components_manager> components_manager;

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&](auto&& version) {
    dispatch_async(dispatch_get_main_queue(), ^{
      components_manager = nullptr;
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  components_manager = std::make_shared<krbn::components_manager>(version_monitor);

  version_monitor->async_start();
  components_manager->async_start();

  CFRunLoopRun();

  version_monitor = nullptr;

  krbn::logger::get_logger()->info("karabiner_grabber is terminated.");

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
