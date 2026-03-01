#pragma once

// `krbn::pressed_keys_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <vector>
#include <unordered_set>

namespace krbn {
class pressed_keys_manager {
public:
  void insert(const momentary_switch_event& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.insert(value);
  }

  void erase(const momentary_switch_event& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.erase(value);
  }

  bool empty(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return entries_.empty();
  }

  std::vector<momentary_switch_event> make_entries_and_clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<momentary_switch_event> result;
    result.reserve(entries_.size());

    for (const auto& e : entries_) {
      result.push_back(e);
    }

    entries_.clear();

    return result;
  }

private:
  std::unordered_set<momentary_switch_event> entries_;
  mutable std::mutex mutex_;
};
} // namespace krbn
