#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "aux_control_button.hpp"
#include "event_type.hpp"
#include "key_code.hpp"
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <pqrs/osx/iokit_service_monitor.hpp>
#include <pqrs/osx/kern_return.hpp>

// Note:
// OS X shares IOHIDSystem among all input devices even the serial_number of IOHIDSystem is same with the one of the input device.
//
// Example:
//   The matched_callback always contains only one IOHIDSystem even if the following devices are connected.
//     * Apple Internal Keyboard / Track
//     * HHKB-BT
//
//   The IOHIDSystem object's serial_number is one of the connected devices.
//
//   But the IOHIDSystem object is shared by all input devices.
//   Thus, the IOHIDGetModifierLockState returns true if caps lock is on in one device.

namespace pqrs {
namespace osx {
namespace iokit_hid_system {
class client final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> opened;
  nod::signal<void(void)> closed;
  nod::signal<void(const std::string&, kern_return)> error_occurred;
  nod::signal<void(kern_return)> post_event_error_occurred;
  nod::signal<void(std::optional<bool>)> caps_lock_state_changed;

  // Methods

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                  caps_lock_state_check_timer_(*this) {
    if (auto matching_dictionary = IOServiceNameMatching(kIOHIDSystemClass)) {
      service_monitor_ = std::make_unique<iokit_service_monitor>(weak_dispatcher_,
                                                                 matching_dictionary);

      service_monitor_->service_matched.connect([this](auto&& registry_entry_id, auto&& service_ptr) {
        close_connection();

        // Use the last matched service.
        open_connection(service_ptr);
      });

      service_monitor_->service_terminated.connect([this](auto&& registry_entry_id) {
        close_connection();

        // Use the next service
        service_monitor_->async_invoke_service_matched();
      });

      CFRelease(matching_dictionary);
    }
  }

  virtual ~client(void) {
    detach_from_dispatcher([this] {
      caps_lock_state_check_timer_.stop();
      close_connection();

      service_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      service_monitor_->async_start();
    });
  }

  void async_start_caps_lock_check_timer(std::chrono::milliseconds interval) {
    enqueue_to_dispatcher([this, interval] {
      last_caps_lock_state_ = std::nullopt;

      caps_lock_state_check_timer_.start(
          [this] {
            auto s = get_modifier_lock_state(kIOHIDCapsLockState);
            if (last_caps_lock_state_ != s) {
              last_caps_lock_state_ = s;
              enqueue_to_dispatcher([this, s] {
                caps_lock_state_changed(s);
              });
            }
          },
          interval);
    });
  }

  void async_set_caps_lock_state(bool state) {
    enqueue_to_dispatcher([this, state] {
      set_modifier_lock_state(kIOHIDCapsLockState, state);
    });
  }

  void async_post_key_code_event(event_type::value_t event_type, key_code::value_t key_code, IOOptionBits flags, bool repeat) {
    NXEventData event{};
    event.key.origCharCode = 0;
    event.key.repeat = repeat;
    event.key.charSet = NX_ASCIISET;
    event.key.charCode = 0;
    event.key.keyCode = type_safe::get(key_code);
    event.key.origCharSet = NX_ASCIISET;
    event.key.keyboardType = 0;

    IOGPoint loc{};
    async_post_event(event_type,
                     loc,
                     event,
                     kNXEventDataVersion,
                     flags,
                     kIOHIDSetGlobalEventFlags);
  }

  void async_post_aux_control_button_event(event_type::value_t event_type, aux_control_button::value_t aux_control_button, IOOptionBits flags, bool repeat) {
    NXEventData event{};
    event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
    event.compound.misc.L[0] = (type_safe::get(aux_control_button) << 16) | (type_safe::get(event_type) << 8) | repeat;

    IOGPoint loc{};
    async_post_event(event_type::system_defined,
                     loc,
                     event,
                     kNXEventDataVersion,
                     flags,
                     kIOHIDSetGlobalEventFlags);
  }

  void async_post_event(event_type::value_t event_type,
                        IOGPoint location,
                        const NXEventData& event_data,
                        UInt32 event_data_version,
                        IOOptionBits event_flags,
                        IOOptionBits options) {
    enqueue_to_dispatcher([this,
                           event_type,
                           location,
                           event_data,
                           event_data_version,
                           event_flags,
                           options] {
      if (connect_) {
        kern_return r = IOHIDPostEvent(*connect_,
                                       type_safe::get(event_type),
                                       location,
                                       &event_data,
                                       event_data_version,
                                       event_flags,
                                       options);
        if (!r) {
          enqueue_to_dispatcher([this, r] {
            post_event_error_occurred(r);
          });
        }
      }
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void open_connection(iokit_object_ptr s) {
    if (!connect_) {
      service_ = s;

      io_connect_t c;
      kern_return r = IOServiceOpen(*service_, mach_task_self(), kIOHIDParamConnectType, &c);
      if (r) {
        connect_ = iokit_object_ptr(c);

        enqueue_to_dispatcher([this] {
          opened();
        });
      } else {
        enqueue_to_dispatcher([this, r] {
          error_occurred("IOServiceOpen is failed.", r);
        });
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void close_connection(void) {
    if (connect_) {
      IOServiceClose(*connect_);
      connect_.reset();

      enqueue_to_dispatcher([this] {
        closed();
      });
    }

    service_.reset();
  }

  // This method is executed in the dispatcher thread.
  std::optional<bool> get_modifier_lock_state(int selector) {
    if (!connect_) {
      return std::nullopt;
    }

    bool state = false;
    kern_return r = IOHIDGetModifierLockState(*connect_, selector, &state);
    if (!r) {
      return std::nullopt;
    }

    return state;
  }

  // This method is executed in the dispatcher thread.
  void set_modifier_lock_state(int selector, bool state) {
    if (!connect_) {
      return;
    }

    IOHIDSetModifierLockState(*connect_, selector, state);
  }

  std::unique_ptr<iokit_service_monitor> service_monitor_;
  iokit_object_ptr service_;
  iokit_object_ptr connect_;

  dispatcher::extra::timer caps_lock_state_check_timer_;
  std::optional<bool> last_caps_lock_state_;
};
} // namespace iokit_hid_system
} // namespace osx
} // namespace pqrs
