#pragma once

// pqrs::iokit_hid_manager v1.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::iokit_hid_manager` can be used safely in a multi-threaded environment.

#include <IOKit/hid/IOHIDDevice.h>
#include <pqrs/osx/iokit_service_monitor.hpp>
#include <unordered_map>
#include <vector>

namespace pqrs {
namespace osx {
class iokit_hid_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(iokit_registry_entry_id, cf_ptr<IOHIDDeviceRef>)> device_detected;
  nod::signal<void(iokit_registry_entry_id)> device_removed;
  nod::signal<void(const std::string&, iokit_return)> error_occurred;

  // Methods

  iokit_hid_manager(const iokit_hid_manager&) = delete;

  iokit_hid_manager(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                    const std::vector<cf_ptr<CFDictionaryRef>>& matching_dictionaries) : dispatcher_client(weak_dispatcher),
                                                                                         matching_dictionaries_(matching_dictionaries) {
  }

  virtual ~iokit_hid_manager(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      start();
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void start(void) {
    for (const auto& matching_dictionary : matching_dictionaries_) {
      if (matching_dictionary) {
        auto monitor = std::make_shared<pqrs::osx::iokit_service_monitor>(weak_dispatcher_,
                                                                          *matching_dictionary);

        monitor->service_detected.connect([this](auto&& registry_entry_id, auto&& service_ptr) {
          if (devices_.find(registry_entry_id) == std::end(devices_)) {
            if (auto device = IOHIDDeviceCreate(kCFAllocatorDefault, *service_ptr)) {
              auto device_ptr = pqrs::cf_ptr<IOHIDDeviceRef>(device);
              devices_[registry_entry_id] = device_ptr;

              enqueue_to_dispatcher([this, registry_entry_id, device_ptr] {
                device_detected(registry_entry_id, device_ptr);
              });

              CFRelease(device);
            }
          }
        });

        monitor->service_removed.connect([this](auto&& registry_entry_id) {
          devices_.erase(registry_entry_id);

          enqueue_to_dispatcher([this, registry_entry_id] {
            device_removed(registry_entry_id);
          });
        });

        monitor->error_occurred.connect([this](auto&& message, auto&& r) {
          enqueue_to_dispatcher([this, message, r] {
            error_occurred(message, r);
          });
        });

        monitor->async_start();

        service_monitors_.push_back(monitor);
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void stop(void) {
    service_monitors_.clear();
    devices_.clear();
  }

  std::vector<cf_ptr<CFDictionaryRef>> matching_dictionaries_;

  std::vector<std::shared_ptr<iokit_service_monitor>> service_monitors_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, pqrs::cf_ptr<IOHIDDeviceRef>> devices_;
};
} // namespace osx
} // namespace pqrs
