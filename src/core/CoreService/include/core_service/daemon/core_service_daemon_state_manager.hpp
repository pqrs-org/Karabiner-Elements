#pragma once

// `core_service_daemon_state_manager` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include <mutex>
#include <nod/nod.hpp>
#include <optional>

namespace krbn {
namespace core_service {
namespace daemon {
class core_service_daemon_state_manager final {
public:
  nod::signal<void(const nlohmann::json&)> changed;

  core_service_daemon_state_manager(void) : state_(nlohmann::json::object()) {
    set_input_monitoring_granted(std::nullopt);
    set_accessibility_process_trusted(std::nullopt);
    set_virtual_hid_device_service_client_connected(std::nullopt);
    set_driver_activated(std::nullopt);
    set_driver_connected(std::nullopt);
    set_driver_version_mismatched(std::nullopt);
    set_karabiner_json_parse_error_message("");
    set_virtual_hid_keyboard_type_not_set(false);
  }

  void set_input_monitoring_granted(std::optional<bool> value) {
    set("input_monitoring_granted", value);
  }

  void set_accessibility_process_trusted(std::optional<bool> value) {
    set("accessibility_process_trusted", value);
  }

  void set_virtual_hid_device_service_client_connected(std::optional<bool> value) {
    set("virtual_hid_device_service_client_connected", value);
  }

  void set_driver_activated(std::optional<bool> value) {
    set("driver_activated", value);
  }

  void set_driver_connected(std::optional<bool> value) {
    set("driver_connected", value);
  }

  void set_driver_version_mismatched(std::optional<bool> value) {
    set("driver_version_mismatched", value);
  }

  void set_karabiner_json_parse_error_message(const std::string& value) {
    set("karabiner_json_parse_error_message", value);
  }

  void set_virtual_hid_keyboard_type_not_set(bool value) {
    set("virtual_hid_keyboard_type_not_set", value);
  }

  nlohmann::json get_state(void) const {
    std::lock_guard<std::mutex> guard(mutex_);

    return state_;
  }

private:
  template <typename T>
  void set(const std::string& key,
           const std::optional<T>& value) {
    std::optional<nlohmann::json> snapshot;

    {
      std::lock_guard<std::mutex> guard(mutex_);

      if (value) {
        if (state_[key] == *value) {
          return;
        }

        state_[key] = *value;
      } else {
        if (!state_.contains(key)) {
          return;
        }

        state_.erase(key);
      }

      snapshot = state_;
    }

    changed(*snapshot);

    if (value) {
      logger::get_logger()->info("core_service_daemon_state {}: {}", key, *value);
    } else {
      logger::get_logger()->info("core_service_daemon_state {}: nullopt", key);
    }
  }

  template <typename T>
  void set(const std::string& key,
           const T& value) {
    std::optional<nlohmann::json> snapshot;

    {
      std::lock_guard<std::mutex> guard(mutex_);

      if (state_[key] == value) {
        return;
      }

      state_[key] = value;
      snapshot = state_;
    }

    changed(*snapshot);

    logger::get_logger()->info("core_service_daemon_state {}: {}", key, value);
  }

  nlohmann::json state_;
  mutable std::mutex mutex_;
};
} // namespace daemon
} // namespace core_service
} // namespace krbn
