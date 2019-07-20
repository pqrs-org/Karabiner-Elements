#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "kextd/kext_loader.hpp"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Check euid
  //

  if (geteuid() != 0) {
    std::cerr << "fatal: karabiner_kextd requires root privilege." << std::endl;
    exit(1);
  }

  //
  // Setup logger
  //

  krbn::logger::set_async_rotating_logger("kextd",
                                          "/var/log/karabiner/kextd.log",
                                          0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_kextd.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      std::string message("Exit since another process is running.");
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 0;
    }
  }

  //
  // Run kext_loader
  //

  std::shared_ptr<krbn::kextd::kext_loader> kext_loader;

  // version_monitor

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&](auto&& version) {
    dispatch_async(dispatch_get_main_queue(), ^{
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  // kext_loader

  kext_loader = std::make_shared<krbn::kextd::kext_loader>(version_monitor);

  kext_loader->kext_loaded.connect([] {
    dispatch_async(dispatch_get_main_queue(), ^{
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  // Start

  version_monitor->async_start();
  kext_loader->async_start();

  CFRunLoopRun();

  kext_loader = nullptr;
  version_monitor = nullptr;

  krbn::logger::get_logger()->info("karabiner_kextd is terminated.");

  return 0;
}
