#pragma once

// `krbn::grabber::hid_event_system_monitor` can be used safely in a multi-threaded environment.

#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_event_system_client.hpp>
#include <pqrs/osx/iokit_service_monitor.hpp>

namespace krbn {
namespace grabber {
class hid_event_system_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_event_system_monitor(const hid_event_system_monitor&) = delete;

  hid_event_system_monitor(void) : dispatcher_client(),
                                   set_property_timer_(*this) {
    if (auto matching_dictionary = IOServiceNameMatching("AppleUserHIDEventDriver")) {
      monitor_ = std::make_unique<pqrs::osx::iokit_service_monitor>(weak_dispatcher_,
                                                                    matching_dictionary);

      monitor_->service_matched.connect([&](auto&& registry_entry_id, auto&& service_ptr) {
        pqrs::osx::iokit_registry_entry entry(service_ptr);

        if (auto serial_number = entry.find_string_property(CFSTR("SerialNumber"))) {
          if (*serial_number == "pqrs.org:Karabiner-DriverKit-VirtualHIDKeyboard") {
            set_property_timer_.start(
                [this, registry_entry_id, serial_number] {
                  logger::get_logger()->info("hid_event_system_monitor set_caps_lock_delay_override for {0}", *serial_number);

                  client_.reload_service_clients();
                  client_.set_caps_lock_delay_override(registry_entry_id, 0);

                  // Retry set_caps_lock_delay_override until the property is set properly.
                  if (auto value = client_.get_caps_lock_delay_override(registry_entry_id)) {
                    if (*value == 0) {
                      set_property_timer_.stop();
                    }
                  }
                },
                std::chrono::milliseconds(3000));
          }
        }
      });

      monitor_->error_occurred.connect([](auto&& message, auto&& kern_return) {
        logger::get_logger()->error("hid_event_system_monitor {0} {1}", message, kern_return);
      });

      monitor_->async_start();
    }
  }

  virtual ~hid_event_system_monitor(void) {
    detach_from_dispatcher([this] {
      set_property_timer_.stop();

      monitor_ = nullptr;
    });
  }

private:
  pqrs::osx::iokit_hid_event_system_client client_;
  std::unique_ptr<pqrs::osx::iokit_service_monitor> monitor_;
  pqrs::dispatcher::extra::timer set_property_timer_;
};
} // namespace grabber
} // namespace krbn
