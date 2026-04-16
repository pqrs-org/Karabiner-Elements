#pragma once

// `core_service_daemon_state_manager` can be used safely in a multi-threaded environment.

#include "types/core_service_daemon_state.hpp"
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace core_service {
namespace daemon {
class core_service_daemon_state_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(const core_service_daemon_state&)> core_service_daemon_state_changed;

  core_service_daemon_state_manager(const core_service_daemon_state_manager&) = delete;

  core_service_daemon_state_manager()
      : dispatcher_client() {
  }

  ~core_service_daemon_state_manager() {
    detach_from_dispatcher();
  }

  std::optional<core_service_permission_check_result> copy_permission_check_result() const {
    std::lock_guard<std::mutex> guard(mutex_);

    return state_.get_permission_check_result();
  }

  void set_permission_check_result(const core_service_permission_check_result& value) {
    update([&value](auto& state) {
      state.set_permission_check_result(value);
    });
  }

  void set_virtual_hid_device_service_client_connected(std::optional<bool> value) {
    update([&value](auto& state) {
      state.set_virtual_hid_device_service_client_connected(value);
    });
  }

  void set_driver_activated(std::optional<bool> value) {
    update([&value](auto& state) {
      state.set_driver_activated(value);
    });
  }

  void set_driver_connected(std::optional<bool> value) {
    update([&value](auto& state) {
      state.set_driver_connected(value);
    });
  }

  void set_driver_version_mismatched(std::optional<bool> value) {
    update([&value](auto& state) {
      state.set_driver_version_mismatched(value);
    });
  }

  void set_virtual_hid_keyboard_type_not_set(std::optional<bool> value) {
    update([&value](auto& state) {
      state.set_virtual_hid_keyboard_type_not_set(value);
    });
  }

  void set_karabiner_json_parse_error_message(const std::string& value) {
    update([&value](auto& state) {
      state.set_karabiner_json_parse_error_message(value);
    });
  }

  core_service_daemon_state copy_state() const {
    std::lock_guard<std::mutex> guard(mutex_);

    return state_;
  }

private:
  template <typename F>
  void update(F&& update_state) {
    core_service_daemon_state state;

    {
      std::lock_guard<std::mutex> guard(mutex_);
      state = state_;
      update_state(state);

      if (state_ == state) {
        return;
      }

      state_ = state;
    }

    enqueue_to_dispatcher([this, state] {
      core_service_daemon_state_changed(state);
    });
  }

  core_service_daemon_state state_;
  mutable std::mutex mutex_;
};
} // namespace daemon
} // namespace core_service
} // namespace krbn
