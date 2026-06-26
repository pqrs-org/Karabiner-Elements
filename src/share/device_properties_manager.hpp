#pragma once

#include "device_properties.hpp"
#include <unordered_map>

namespace krbn {
class device_properties_manager final {
public:
  device_properties_manager(const device_properties_manager&) = delete;

  device_properties_manager() {
  }

  [[nodiscard]] const std::unordered_map<device_id, pqrs::not_null_shared_ptr_t<device_properties>>& get_map() const {
    return map_;
  }

  void insert(device_id key,
              pqrs::not_null_shared_ptr_t<device_properties> value) {
    map_.insert_or_assign(key,
                          value);
  }

  void erase(device_id key) {
    map_.erase(key);
  }

  void clear() {
    map_.clear();
  }

  [[nodiscard]] std::shared_ptr<device_properties> find(device_id key) const {
    auto it = map_.find(key);
    if (it != std::end(map_)) {
      return pqrs::unwrap_not_null(it->second);
    }
    return nullptr;
  }

private:
  std::unordered_map<device_id, pqrs::not_null_shared_ptr_t<device_properties>> map_;
};
} // namespace krbn
