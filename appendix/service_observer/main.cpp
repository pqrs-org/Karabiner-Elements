#include "iokit_utility.hpp"
#include "service_observer.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace {
void matched_callback(io_iterator_t iterator) {
  while (auto service = IOIteratorNext(iterator)) {
    if (auto registry_entry_id = krbn::iokit_utility::find_registry_entry_id(service)) {
      std::cout << "matched_callback " << *registry_entry_id << std::endl;
    }
    IOObjectRelease(service);
  }
}

void terminated_callback(io_iterator_t iterator) {
  while (auto service = IOIteratorNext(iterator)) {
    if (auto registry_entry_id = krbn::iokit_utility::find_registry_entry_id(service)) {
      std::cout << "terminated_callback " << *registry_entry_id << std::endl;
    }
    IOObjectRelease(service);
  }
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  std::unique_ptr<krbn::service_observer> service_observer;

  // auto service_name = "org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDPointing_v040900";
  auto service_name = "org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDRoot_v040900";

  if (auto matching_dictionary = IOServiceNameMatching(service_name)) {
    service_observer = std::make_unique<krbn::service_observer>(matching_dictionary,
                                                                matched_callback,
                                                                terminated_callback);
    CFRelease(matching_dictionary);
  }

  CFRunLoopRun();

  return 0;
}
