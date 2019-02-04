#pragma once

// `krbn::pressed_keys_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <mpark/variant.hpp>
#include <unordered_set>

namespace krbn {
class pressed_keys_manager {
public:
  template <typename T>
  void insert(T value) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.insert(value);
  }

  template <typename T>
  void erase(T value) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.erase(value);
  }

  bool empty(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return entries_.empty();
  }

private:
  using entry_t = mpark::variant<key_code,
                                 consumer_key_code,
                                 pointing_button>;
  std::unordered_set<entry_t, std::hash<entry_t>> entries_;

  mutable std::mutex mutex_;
};
} // namespace krbn
