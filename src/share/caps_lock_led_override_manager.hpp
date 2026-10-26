#pragma once

#include "types/led_state.hpp"
#include "types/set_caps_lock_led_value.hpp"
#include <memory>
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <utility>

// `krbn::caps_lock_led_override_manager` can be used safely in a multi-threaded environment.

namespace krbn {
// Holds the caps lock LED state requested by the `set_caps_lock_led` event of complex modifications.
//
// While an override is set, the caps lock LED is forced to that state regardless of the actual
// caps lock state. `set_caps_lock_led_value::automatic` removes the override so that the LED follows
// the caps lock state again.
//
// The LED is owned by Karabiner-Core-Service (device_grabber), so this manager lives in the daemon
// and `post_event_to_virtual_devices` calls it directly instead of forwarding the event to
// Karabiner-Console-User-Server.
class caps_lock_led_override_manager final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  // The new override. std::nullopt means that no override is set.
  nod::signal<void(std::optional<led_state>)> override_changed;

  explicit caps_lock_led_override_manager(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher =
                                              pqrs::dispatcher::extra::get_shared_dispatcher())
      : dispatcher_client(std::move(weak_dispatcher)) {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~caps_lock_led_override_manager() override {
    detach_from_dispatcher();
  }

  [[nodiscard]] std::optional<led_state> get_override() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return override_;
  }

  void async_set_override(set_caps_lock_led_value value) {
    enqueue_to_dispatcher([this, value] {
      switch (value) {
        case set_caps_lock_led_value::on:
          set_override(led_state::on);
          break;
        case set_caps_lock_led_value::off:
          set_override(led_state::off);
          break;
        case set_caps_lock_led_value::automatic:
          set_override(std::nullopt);
          break;
      }
    });
  }

  void async_clear_override() {
    enqueue_to_dispatcher([this] {
      set_override(std::nullopt);
    });
  }

private:
  void set_override(std::optional<led_state> value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (override_ == value) {
        return;
      }

      override_ = value;
    }

    override_changed(value);
  }

  mutable std::mutex mutex_;
  std::optional<led_state> override_;
};
} // namespace krbn
