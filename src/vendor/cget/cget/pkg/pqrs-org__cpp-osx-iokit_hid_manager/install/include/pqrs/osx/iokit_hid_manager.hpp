#pragma once

// pqrs::osx::iokit_hid_manager v4.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::iokit_hid_manager` can be used safely in a multi-threaded environment.

#include <IOKit/hid/IOHIDDevice.h>
#include <pqrs/cf/number.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_service_monitor.hpp>
#include <unordered_map>
#include <vector>

namespace pqrs {
namespace osx {
class iokit_hid_manager final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(iokit_registry_entry_id::value_t, cf::cf_ptr<IOHIDDeviceRef>)> device_matched;
  nod::signal<void(iokit_registry_entry_id::value_t)> device_terminated;
  nod::signal<void(const std::string&, kern_return)> error_occurred;

  // Methods

  iokit_hid_manager(const iokit_hid_manager&) = delete;

  iokit_hid_manager(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                    const std::vector<cf::cf_ptr<CFDictionaryRef>>& matching_dictionaries,
                    pqrs::dispatcher::duration device_matched_delay = pqrs::dispatcher::duration(0)) : dispatcher_client(weak_dispatcher),
                                                                                                       matching_dictionaries_(matching_dictionaries),
                                                                                                       device_matched_delay_(device_matched_delay) {
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

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_rescan(void) {
    enqueue_to_dispatcher([this] {
      rescan();
    });
  }

  void async_set_device_matched_delay(pqrs::dispatcher::duration value) {
    enqueue_to_dispatcher([this, value] {
      device_matched_delay_ = value;
    });
  }

  static cf::cf_ptr<CFDictionaryRef> make_matching_dictionary(hid::usage_page::value_t hid_usage_page,
                                                              hid::usage::value_t hid_usage) {
    cf::cf_ptr<CFDictionaryRef> result;

    if (auto matching_dictionary = IOServiceMatching(kIOHIDDeviceKey)) {
      if (auto number = cf::make_cf_number(type_safe::get(hid_usage_page))) {
        CFDictionarySetValue(matching_dictionary,
                             CFSTR(kIOHIDDeviceUsagePageKey),
                             *number);
      }
      if (auto number = cf::make_cf_number(type_safe::get(hid_usage))) {
        CFDictionarySetValue(matching_dictionary,
                             CFSTR(kIOHIDDeviceUsageKey),
                             *number);
      }

      result = matching_dictionary;

      CFRelease(matching_dictionary);
    }

    return result;
  }

  static cf::cf_ptr<CFDictionaryRef> make_matching_dictionary(hid::usage_page::value_t hid_usage_page) {
    cf::cf_ptr<CFDictionaryRef> result;

    if (auto matching_dictionary = IOServiceMatching(kIOHIDDeviceKey)) {
      if (auto number = cf::make_cf_number(type_safe::get(hid_usage_page))) {
        CFDictionarySetValue(matching_dictionary,
                             CFSTR(kIOHIDDeviceUsagePageKey),
                             *number);
      }

      result = matching_dictionary;

      CFRelease(matching_dictionary);
    }

    return result;
  }

private:
  // This method is executed in the dispatcher thread.
  void start(void) {
    if (!service_monitors_.empty()) {
      // already started
      return;
    }

    for (const auto& matching_dictionary : matching_dictionaries_) {
      if (matching_dictionary) {
        auto monitor = std::make_shared<iokit_service_monitor>(weak_dispatcher_,
                                                               *matching_dictionary);

        monitor->service_matched.connect([this](auto&& registry_entry_id, auto&& service_ptr) {
          if (devices_.find(registry_entry_id) == std::end(devices_)) {
            if (auto device = IOHIDDeviceCreate(kCFAllocatorDefault, *service_ptr)) {
              auto device_ptr = cf::cf_ptr<IOHIDDeviceRef>(device);
              devices_[registry_entry_id] = device_ptr;

              auto when = when_now() + device_matched_delay_;
              enqueue_to_dispatcher(
                  [this, registry_entry_id, device_ptr] {
                    auto it = devices_.find(registry_entry_id);
                    if (it == std::end(devices_)) {
                      // device is already terminated.
                      return;
                    }

                    enqueue_to_dispatcher([this, registry_entry_id, device_ptr] {
                      device_matched(registry_entry_id, device_ptr);
                    });
                    device_matched_called_ids_.insert(registry_entry_id);
                  },
                  when);

              CFRelease(device);
            }
          }
        });

        monitor->service_terminated.connect([this](auto&& registry_entry_id) {
          auto it = devices_.find(registry_entry_id);
          if (it != std::end(devices_)) {
            devices_.erase(registry_entry_id);

            auto it = device_matched_called_ids_.find(registry_entry_id);
            if (it == std::end(device_matched_called_ids_)) {
              // `device_matched` is not called yet.
              // (The device is disconnected before `deviec_matched` is called.)
              return;
            }

            enqueue_to_dispatcher([this, registry_entry_id] {
              device_terminated(registry_entry_id);
            });
            device_matched_called_ids_.erase(registry_entry_id);
          }
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
    device_matched_called_ids_.clear();
  }

  void rescan(void) {
    for (const auto& m : service_monitors_) {
      m->async_invoke_service_matched();
    }
  }

  std::vector<cf::cf_ptr<CFDictionaryRef>> matching_dictionaries_;
  pqrs::dispatcher::duration device_matched_delay_;
  std::vector<std::shared_ptr<iokit_service_monitor>> service_monitors_;
  std::unordered_map<iokit_registry_entry_id::value_t, cf::cf_ptr<IOHIDDeviceRef>> devices_;
  std::unordered_set<iokit_registry_entry_id::value_t> device_matched_called_ids_;
};
} // namespace osx
} // namespace pqrs
