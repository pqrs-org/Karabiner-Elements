#pragma once

// `krbn::device_properties_manager` can be used safely in a multi-threaded environment.

#include "device_properties.hpp"
#include <unordered_map>

namespace krbn {
class device_properties_manager final {
public:
  device_properties_manager(const device_properties_manager&) = delete;

  device_properties_manager(void) {
  }

  void insert(device_id device_id,
              std::shared_ptr<device_properties> device_properties) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_[device_id] = device_properties;
  }

  void erase(device_id device_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_.erase(device_id);
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_.clear();
  }

  std::shared_ptr<device_properties> find(device_id device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(device_id);
    if (it != std::end(map_)) {
      return it->second;
    }
    return nullptr;
  }

private:
  std::unordered_map<device_id, std::shared_ptr<device_properties>> map_;
  mutable std::mutex mutex_;
};
} // namespace krbn
