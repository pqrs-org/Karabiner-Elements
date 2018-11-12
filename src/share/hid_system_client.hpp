#pragma once

#include "boost_defs.hpp"

#include "iokit_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <boost/signals2.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/iokit_service_monitor.hpp>

namespace krbn {
class hid_system_client final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(boost::optional<bool>)> caps_lock_state_changed;

  // Methods

  hid_system_client(const hid_system_client&) = delete;

  // Note:
  // OS X shares IOHIDSystem among all input devices even the serial_number of IOHIDSystem is same with the one of the input device.
  //
  // Example:
  //   The matched_callback always contains only one IOHIDSystem even if the following devices are connected.
  //     * Apple Internal Keyboard / Track
  //     * HHKB-BT
  //     * org.pqrs.driver.VirtualHIDKeyboard
  //
  //   The IOHIDSystem object's serial_number is one of the connected devices.
  //
  //   But the IOHIDSystem object is shared by all input devices.
  //   Thus, the IOHIDGetModifierLockState returns true if caps lock is on in one device.

  hid_system_client(void) : dispatcher_client(),
                            service_(IO_OBJECT_NULL),
                            connect_(IO_OBJECT_NULL),
                            caps_lock_state_check_timer_(*this) {
    if (auto matching_dictionary = IOServiceNameMatching(kIOHIDSystemClass)) {
      service_monitor_ = std::make_unique<pqrs::osx::iokit_service_monitor>(weak_dispatcher_,
                                                                            matching_dictionary);

      service_monitor_->service_detected.connect([this](auto&& registry_entry_id, auto&& service_ptr) {
        close_connection();

        // Use the last matched service.
        open_connection(*service_ptr);
      });

      service_monitor_->service_removed.connect([this](auto&& registry_entry_id) {
        close_connection();

        // Use the next service
        service_monitor_->async_invoke_service_detected();
      });

      service_monitor_->async_start();

      CFRelease(matching_dictionary);
    }
  }

  virtual ~hid_system_client(void) {
    detach_from_dispatcher([this] {
      caps_lock_state_check_timer_.stop();
      close_connection();

      service_monitor_ = nullptr;
    });
  }

  void async_start_caps_lock_check_timer(std::chrono::milliseconds interval) {
    enqueue_to_dispatcher([this, interval] {
      last_caps_lock_state_ = boost::none;

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

private:
  // This method is executed in the dispatcher thread.
  void open_connection(io_service_t s) {
    service_ = s;
    IOObjectRetain(service_);

    pqrs::osx::iokit_return r = IOServiceOpen(service_, mach_task_self(), kIOHIDParamConnectType, &connect_);
    if (r) {
      logger::get_logger().info("hid_system_client is opened.");

    } else {
      logger::get_logger().error("hid_system_client::open_connection is failed: {0}",
                                 r.to_string());
      connect_ = IO_OBJECT_NULL;
    }
  }

  // This method is executed in the dispatcher thread.
  void close_connection(void) {
    if (connect_) {
      pqrs::osx::iokit_return r = IOServiceClose(connect_);
      if (!r) {
        logger::get_logger().error("hid_system_client::close_connection error: {0}",
                                   r.to_string());
      }
      connect_ = IO_OBJECT_NULL;
    }

    logger::get_logger().info("hid_system_client is closed.");

    if (service_) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }
  }

  // This method is executed in the dispatcher thread.
  boost::optional<bool> get_modifier_lock_state(int selector) {
    if (!connect_) {
      logger::get_logger().error("hid_system_client::get_modifier_lock_state connect_ is null.");
      return boost::none;
    }

    bool state = false;
    pqrs::osx::iokit_return r = IOHIDGetModifierLockState(connect_, selector, &state);
    if (!r) {
      logger::get_logger().error("IOHIDGetModifierLockState is failed: {0}",
                                 r.to_string());
      return boost::none;
    }

    return state;
  }

  // This method is executed in the dispatcher thread.
  void set_modifier_lock_state(int selector, bool state) {
    if (!connect_) {
      logger::get_logger().error("hid_system_client::set_modifier_lock_state connect_ is null.");
      return;
    }

    pqrs::osx::iokit_return r = IOHIDSetModifierLockState(connect_, selector, state);
    if (!r) {
      logger::get_logger().error("IOHIDSetModifierLockState is failed: {0}",
                                 r.to_string());
    }
  }

  std::unique_ptr<pqrs::osx::iokit_service_monitor> service_monitor_;
  io_service_t service_;
  io_connect_t connect_;

  pqrs::dispatcher::extra::timer caps_lock_state_check_timer_;
  boost::optional<bool> last_caps_lock_state_;
};
} // namespace krbn
