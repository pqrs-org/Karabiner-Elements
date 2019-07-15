#pragma once

#include <pqrs/osx/iokit_hid_device_open_checker.hpp>

namespace krbn {
namespace iokit_hid_device_open_checker_utility {
inline void run_checker(void) {
  auto global_wait = pqrs::make_thread_wait();

  std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::osx::iokit_hid_usage_page_generic_desktop,
          pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),

      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::osx::iokit_hid_usage_page_generic_desktop,
          pqrs::osx::iokit_hid_usage_generic_desktop_mouse),

      pqrs::osx::iokit_hid_manager::make_matching_dictionary(
          pqrs::osx::iokit_hid_usage_page_generic_desktop,
          pqrs::osx::iokit_hid_usage_generic_desktop_pointer),
  };

  auto checker = std::make_unique<pqrs::osx::iokit_hid_device_open_checker>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                            matching_dictionaries);

  checker->device_open_permitted.connect([&] {
    global_wait->notify();
  });

  checker->device_open_forbidden.connect([&] {
    global_wait->notify();
  });

  checker->async_start();

  global_wait->wait_notice();
}
} // namespace iokit_hid_device_open_checker_utility
} // namespace krbn
