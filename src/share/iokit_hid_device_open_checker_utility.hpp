#pragma once

#include "logger.hpp"
#include "run_loop_thread_utility.hpp"
#include <pqrs/osx/iokit_hid_device_open_checker.hpp>

namespace krbn {
namespace iokit_hid_device_open_checker_utility {
inline bool run_checker(void) {
  bool result = true;

  auto global_wait = pqrs::make_thread_wait();

  std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
      // Devices other than keyboards don’t require input monitoring permission.
      // So if gamepads or other thinks are in `matching_dictionaries`,
      // the `iokit_hid_device_open_checker` might incorrectly assume input monitoring is allowed,
      // even when it's not.
      //
      // This happens because `iokit_hid_device_open_checker` checks whether the device open call returns
      // `not_permitted` to determine permission status.
      //
      // So we should include only the `generic_desktop::keyboard` here.

      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::hid::usage_page::generic_desktop,
          pqrs::hid::usage::generic_desktop::keyboard),
  };

  auto checker = std::make_unique<pqrs::osx::iokit_hid_device_open_checker>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                            pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
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
