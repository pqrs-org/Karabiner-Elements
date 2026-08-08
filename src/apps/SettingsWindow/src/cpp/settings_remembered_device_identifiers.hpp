#pragma once

#include "connected_devices.hpp"
#include <algorithm>
#include <mutex>
#include <vector>

// Keeps identifiers for every device observed during this process, including
// devices that are currently disconnected. Its process lifetime is independent
// of settings_components_manager, which is recreated across sleep and wake.
class settings_remembered_device_identifiers final {
public:
  settings_remembered_device_identifiers(const settings_remembered_device_identifiers&) = delete;
  settings_remembered_device_identifiers& operator=(const settings_remembered_device_identifiers&) = delete;

  [[nodiscard]] static settings_remembered_device_identifiers& get_instance() {
    static settings_remembered_device_identifiers instance;
    return instance;
  }

  bool remember_connected_devices(const krbn::connected_devices& connected_devices) {
    auto changed = false;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& device : connected_devices.get_devices()) {
      const auto& identifiers = device->get_device_identifiers();
      if (std::ranges::find(device_identifiers_, identifiers) == std::end(device_identifiers_)) {
        device_identifiers_.push_back(identifiers);
        changed = true;
      }
    }

    return changed;
  }

  [[nodiscard]] std::vector<krbn::device_identifiers> get_device_identifiers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return device_identifiers_;
  }

private:
  settings_remembered_device_identifiers() = default;

  mutable std::mutex mutex_;
  std::vector<krbn::device_identifiers> device_identifiers_;
};
