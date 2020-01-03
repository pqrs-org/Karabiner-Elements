#pragma once

// pqrs::osx::iokit_hid_device_open_checker v1.2

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_return.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_device_open_checker final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> device_open_permitted;
  nod::signal<void(void)> device_open_forbidden;

  // Methods

  iokit_hid_device_open_checker(const iokit_hid_device_open_checker&) = delete;

  iokit_hid_device_open_checker(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                                const std::vector<cf::cf_ptr<CFDictionaryRef>>& matching_dictionaries,
                                pqrs::dispatcher::duration device_matched_delay = pqrs::dispatcher::duration(0)) : dispatcher_client(weak_dispatcher),
                                                                                                                   permitted_(false),
                                                                                                                   timer_(*this) {
    iokit_hid_manager_ = std::make_unique<iokit_hid_manager>(weak_dispatcher,
                                                             matching_dictionaries,
                                                             device_matched_delay);

    iokit_hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device) {
      if (device) {
        wait_ = 5;

        iokit_return r = IOHIDDeviceOpen(*device, kIOHIDOptionsTypeNone);

        if (!r.not_permitted()) {
          if (!permitted_) {
            permitted_ = true;
            enqueue_to_dispatcher([this] {
              device_open_permitted();
            });
          }
        }
      }
    });
  }

  virtual ~iokit_hid_device_open_checker(void) {
    detach_from_dispatcher([this] {
      timer_.stop();

      iokit_hid_manager_ = nullptr;
    });
  }

  void async_start(void) {
    if (iokit_hid_manager_) {
      iokit_hid_manager_->async_start();

      timer_.start(
          [this] {
            // Keep `timer_` while no device is matched.

            if (!wait_) {
              return;
            }

            // Keep `timer_` for a while after a device is matched.

            if (*wait_ > 0) {
              --(*wait_);
              return;
            }

            iokit_hid_manager_ = nullptr;

            if (!permitted_) {
              enqueue_to_dispatcher([this] {
                device_open_forbidden();
              });
            }

            timer_.stop();
          },
          std::chrono::milliseconds(1000));
    }
  }

private:
  std::unique_ptr<iokit_hid_manager> iokit_hid_manager_;
  bool permitted_;
  std::optional<size_t> wait_;
  pqrs::dispatcher::extra::timer timer_;
};
} // namespace osx
} // namespace pqrs
