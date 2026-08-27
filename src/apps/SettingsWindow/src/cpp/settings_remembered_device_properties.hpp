#pragma once

#include "connected_devices.hpp"
#include <algorithm>
#include <mutex>
#include <vector>

// Keeps properties for every device observed during this process, including
// devices that are currently disconnected. Its process lifetime is independent
// of settings_components_manager, which is recreated across sleep and wake.
class settings_remembered_device_properties final {
public:
  settings_remembered_device_properties(const settings_remembered_device_properties&) = delete;
  settings_remembered_device_properties& operator=(const settings_remembered_device_properties&) = delete;

  [[nodiscard]] static settings_remembered_device_properties& get_instance() {
    static settings_remembered_device_properties instance;
    return instance;
  }

  bool remember_connected_devices(const krbn::connected_devices& connected_devices) {
    auto changed = false;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& device : connected_devices.get_devices()) {
      const auto& identifiers = device->get_device_identifiers();
      auto it = std::ranges::find_if(
          device_properties_,
          [&](const auto& p) {
            return p->get_device_identifiers() == identifiers;
          });
      if (it == std::end(device_properties_)) {
        device_properties_.push_back(device);
        changed = true;
      }
    }

    return changed;
  }

  [[nodiscard]] std::vector<pqrs::not_null_shared_ptr_t<krbn::device_properties>> get_device_properties() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return device_properties_;
  }

private:
  settings_remembered_device_properties() = default;

  mutable std::mutex mutex_;
  std::vector<pqrs::not_null_shared_ptr_t<krbn::device_properties>> device_properties_;
};
