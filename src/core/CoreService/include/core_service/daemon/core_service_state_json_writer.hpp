#pragma once

// `core_service_state_json_writer` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include <mutex>
#include <optional>

namespace krbn {
namespace core_service {
namespace daemon {
class core_service_state_json_writer final {
public:
  core_service_state_json_writer(void) : state_(nlohmann::json::object()) {
    set_hid_device_open_permitted(std::nullopt);
    set_accessibility_process_trusted(std::nullopt);
    set_virtual_hid_device_service_client_connected(std::nullopt);
    set_driver_activated(std::nullopt);
    set_driver_connected(std::nullopt);
    set_driver_version_mismatched(std::nullopt);

    sync_save();
  }

  void set_hid_device_open_permitted(std::optional<bool> value) {
    set("hid_device_open_permitted", value);
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

private:
  template <typename T>
  void set(const std::string& key,
           const std::optional<T>& value) {
    std::lock_guard<std::mutex> guard(mutex_);

    if (value) {
      if (state_[key] == *value) {
        return;
      }

      state_[key] = *value;
      sync_save();

      logger::get_logger()->info("core_service_state {}: {}", key, *value);

    } else {
      if (!state_.contains(key)) {
        return;
      }

      state_.erase(key);
      sync_save();

      logger::get_logger()->info("core_service_state {}: nullopt", key);
    }
  }

  void sync_save(void) {
    json_writer::sync_save_to_file(state_,
                                   constants::get_core_service_state_json_file_path(),
                                   0755,
                                   0644);
  }

  nlohmann::json state_;
  std::mutex mutex_;
};
} // namespace daemon
} // namespace core_service
} // namespace krbn
