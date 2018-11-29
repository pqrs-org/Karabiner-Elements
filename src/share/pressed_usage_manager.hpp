#pragma once

// `krbn::pressed_usage_manager` can be used safely in a multi-threaded environment.

#include <pqrs/osx/iokit_types.hpp>
#include <unordered_set>

namespace krbn {
class pressed_usage_manager {
public:
  void insert(pqrs::osx::iokit_hid_usage_page hid_usage_page,
              pqrs::osx::iokit_hid_usage hid_usage) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.insert(std::make_pair(hid_usage_page,
                                   hid_usage));
  }

  void erase(pqrs::osx::iokit_hid_usage_page hid_usage_page,
             pqrs::osx::iokit_hid_usage hid_usage) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_.erase(std::make_pair(hid_usage_page,
                                  hid_usage));
  }

  bool empty(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return entries_.empty();
  }

private:
  std::unordered_set<std::pair<pqrs::osx::iokit_hid_usage_page, pqrs::osx::iokit_hid_usage>> entries_;
  mutable std::mutex mutex_;
};
} // namespace krbn
