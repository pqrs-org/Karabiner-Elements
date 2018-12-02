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

  void insert(device_id key,
              std::shared_ptr<device_properties> value) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_[key] = value;
  }

  void insert(device_id key,
              const device_properties& value) {
    insert(key,
           std::make_shared<device_properties>(value));
  }

  void erase(device_id key) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_.erase(key);
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    map_.clear();
  }

  std::shared_ptr<device_properties> find(device_id key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);
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
