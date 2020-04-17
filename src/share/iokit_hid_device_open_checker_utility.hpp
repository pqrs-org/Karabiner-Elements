#pragma once

#include "logger.hpp"
#include <pqrs/osx/iokit_hid_device_open_checker.hpp>

namespace krbn {
namespace iokit_hid_device_open_checker_utility {
inline bool run_checker(void) {
  bool result = true;

  auto global_wait = pqrs::make_thread_wait();

  std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::hid::usage_page::generic_desktop,
          pqrs::hid::usage::generic_desktop::keyboard),

      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::hid::usage_page::generic_desktop,
          pqrs::hid::usage::generic_desktop::mouse),

      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::hid::usage_page::generic_desktop,
          pqrs::hid::usage::generic_desktop::pointer),
  };

  auto checker = std::make_unique<pqrs::osx::iokit_hid_device_open_checker>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                            matching_dictionaries);

  checker->device_open_permitted.connect([&] {
    logger::get_logger()->info("device_open_permitted");
    result = true;
    global_wait->notify();
  });

  checker->device_open_forbidden.connect([&] {
    logger::get_logger()->warn("device_open_forbidden");
    result = false;
    global_wait->notify();
  });

  checker->async_start();

  global_wait->wait_notice();

  return result;
}
} // namespace iokit_hid_device_open_checker_utility
} // namespace krbn
