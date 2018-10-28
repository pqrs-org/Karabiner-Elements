#include "dispatcher_utility.hpp"
#include "iokit_utility.hpp"
#include "monitor/service_monitor.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  std::unique_ptr<krbn::monitor::service_monitor::service_monitor> service_monitor;

  auto service_name = "IOHIDSystem";

  if (auto matching_dictionary = IOServiceNameMatching(service_name)) {
    service_monitor = std::make_unique<krbn::monitor::service_monitor::service_monitor>(matching_dictionary);

    service_monitor->service_detected.connect([](auto&& services) {
      for (const auto& s : services->get_services()) {
        if (auto registry_entry_id = krbn::iokit_utility::find_registry_entry_id(s)) {
          std::cout << "matched_callback " << static_cast<uint64_t>(*registry_entry_id) << std::endl;
        }
      }
    });

    service_monitor->service_removed.connect([](auto&& services) {
      for (const auto& s : services->get_services()) {
        if (auto registry_entry_id = krbn::iokit_utility::find_registry_entry_id(s)) {
          std::cout << "terminated_callback " << static_cast<uint64_t>(*registry_entry_id) << std::endl;
        }
      }
    });

    service_monitor->async_start();

    CFRelease(matching_dictionary);
  }

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  service_monitor = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
