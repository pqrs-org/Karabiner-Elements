#include "apple_notification_center.hpp"
#include "components_manager.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "grabber_alerts_manager.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include "thread_utility.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
    exit(1);
  }

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  krbn::thread_utility::register_main_thread();

  // Setup logger

  krbn::logger::set_async_rotating_logger("grabber",
                                          "/var/log/karabiner/grabber.log",
                                          0755);

  krbn::logger::get_logger().info("version {0}", karabiner_version);

  // Check another process

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_grabber.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      std::string message("Exit since another process is running.");
      krbn::logger::get_logger().info(message);
      std::cerr << message << std::endl;
      return 0;
    }
  }

  // Load kexts

  krbn::grabber_alerts_manager::enable_json_output(krbn::constants::get_grabber_alerts_json_file_path());
  krbn::grabber_alerts_manager::async_save_to_file();

  while (true) {
    std::stringstream ss;
    ss << "/sbin/kextload '"
       << "/Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice/Extensions/"
       << pqrs::karabiner_virtual_hid_device::get_kernel_extension_name()
       << "'";
    krbn::logger::get_logger().info(ss.str());
    int exit_status = system(ss.str().c_str());
    exit_status >>= 8;
    krbn::logger::get_logger().info("kextload exit status: {0}", exit_status);
    if (exit_status == 27) {
      // kextload is blocked by macOS.
      // https://developer.apple.com/library/content/technotes/tn2459/_index.html
      krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, true);
      std::this_thread::sleep_for(std::chrono::seconds(3));
      continue;
    }
    break;
  }
  krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, false);

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

  krbn::manipulator::manipulator_timer::get_instance().enable();
  krbn::components_manager components_manager;

  krbn::apple_notification_center::post_distributed_notification_to_all_sessions(krbn::constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  return 0;
}
