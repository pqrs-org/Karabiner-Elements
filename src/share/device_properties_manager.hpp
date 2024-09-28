#pragma once

#include "device_properties.hpp"
#include <unordered_map>

namespace krbn {
class device_properties_manager final {
public:
  device_properties_manager(const device_properties_manager&) = delete;

  device_properties_manager(void) {
  }

  const std::unordered_map<device_id, std::shared_ptr<device_properties>>& get_map(void) const {
    return map_;
  }

  void insert(device_id key,
              gsl::not_null<std::shared_ptr<device_properties>> value) {
    map_[key] = value;
  }

  void erase(device_id key) {
    map_.erase(key);
  }

  void clear(void) {
    map_.clear();
  }

  std::shared_ptr<device_properties> find(device_id key) const {
    auto it = map_.find(key);
    if (it != std::end(map_)) {
      return it->second;
    }
    return nullptr;
  }

private:
  // std::unordered_map<T1, gsl::not_null<T2>> results in a build error because gsl::not_null<T2> is not hashable. Therefore, we will not use gsl::not_null here.
  std::unordered_map<device_id, std::shared_ptr<device_properties>> map_;
};
} // namespace krbn
