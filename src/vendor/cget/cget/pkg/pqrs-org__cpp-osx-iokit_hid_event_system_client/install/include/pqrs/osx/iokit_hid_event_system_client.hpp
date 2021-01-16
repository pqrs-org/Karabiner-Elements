#pragma once

// pqrs::osx::iokit_hid_event_system_client v1.1

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hidsystem/IOHIDEventSystemClient.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/IOHIDServiceClient.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <unordered_map>
#include <vector>

namespace pqrs {
namespace osx {
class iokit_hid_event_system_client final {
public:
  iokit_hid_event_system_client(const iokit_hid_event_system_client&) = delete;

  iokit_hid_event_system_client(void) {
    reload_service_clients();
  }

  virtual ~iokit_hid_event_system_client(void) {
    clear();
  }

  void reload_service_clients(void) {
    clear();

    if (auto c = IOHIDEventSystemClientCreateSimpleClient(kCFAllocatorDefault)) {
      event_system_client_ = c;

      CFRelease(c);
    }

    if (event_system_client_) {
      if (CFArrayRef services = IOHIDEventSystemClientCopyServices(*event_system_client_)) {
        for (CFIndex i = 0; i < CFArrayGetCount(services); i++) {
          if (auto c = cf::get_cf_array_value<IOHIDServiceClientRef>(services, i)) {
            if (auto id = cf::make_number<int64_t>(IOHIDServiceClientGetRegistryID(c))) {
              iokit_registry_entry_id::value_t registry_entry_id(*id);
              service_clients_[registry_entry_id] = cf::cf_ptr(c);
            }
          }
        }

        CFRelease(services);
      }
    }
  }

  std::vector<iokit_registry_entry_id::value_t> make_registry_entry_ids(void) const {
    std::vector<iokit_registry_entry_id::value_t> registry_entry_ids;

    for (const auto& it : service_clients_) {
      registry_entry_ids.push_back(it.first);
    }

    return registry_entry_ids;
  }

  template <typename T>
  std::optional<T> get_integer_property(iokit_registry_entry_id::value_t registry_entry_id,
                                        CFStringRef key) const {
    std::optional<T> result;

    auto it = service_clients_.find(registry_entry_id);
    if (it != std::end(service_clients_)) {
      if (auto value = IOHIDServiceClientCopyProperty(*(it->second), key)) {
        result = cf::make_number<T>(value);
        CFRelease(value);
      }
    }

    return result;
  }

  template <typename T>
  void set_integer_property(iokit_registry_entry_id::value_t registry_entry_id,
                            CFStringRef key,
                            T value) const {
    auto it = service_clients_.find(registry_entry_id);
    if (it != std::end(service_clients_)) {
      if (auto v = cf::make_cf_number(value)) {
        IOHIDServiceClientSetProperty(*(it->second), key, *v);
      }
    }
  }

  std::optional<int64_t> get_initial_key_repeat(iokit_registry_entry_id::value_t registry_entry_id) const {
    return get_integer_property<int64_t>(registry_entry_id,
                                         CFSTR(kIOHIDServiceInitialKeyRepeatDelayKey));
  }

  void set_initial_key_repeat(iokit_registry_entry_id::value_t registry_entry_id,
                              int64_t value) const {
    set_integer_property(registry_entry_id,
                         CFSTR(kIOHIDServiceInitialKeyRepeatDelayKey),
                         value);
  }

  std::optional<int64_t> get_key_repeat(iokit_registry_entry_id::value_t registry_entry_id) const {
    return get_integer_property<int64_t>(registry_entry_id,
                                         CFSTR(kIOHIDServiceKeyRepeatDelayKey));
  }

  void set_key_repeat(iokit_registry_entry_id::value_t registry_entry_id,
                      int64_t value) const {
    set_integer_property(registry_entry_id,
                         CFSTR(kIOHIDServiceKeyRepeatDelayKey),
                         value);
  }

  std::optional<int64_t> get_caps_lock_delay_override(iokit_registry_entry_id::value_t registry_entry_id) const {
    return get_integer_property<int64_t>(registry_entry_id,
                                         CFSTR(kIOHIDKeyboardCapsLockDelayOverrideKey));
  }

  void set_caps_lock_delay_override(iokit_registry_entry_id::value_t registry_entry_id,
                                    int32_t value) const {
    set_integer_property(registry_entry_id,
                         CFSTR(kIOHIDKeyboardCapsLockDelayOverrideKey),
                         value);
  }

private:
  void clear(void) {
    service_clients_.clear();
    event_system_client_.reset();
  }

  cf::cf_ptr<IOHIDEventSystemClientRef> event_system_client_;
  std::unordered_map<iokit_registry_entry_id::value_t, cf::cf_ptr<IOHIDServiceClientRef>> service_clients_;
};
} // namespace osx
} // namespace pqrs
