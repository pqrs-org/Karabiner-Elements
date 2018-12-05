#pragma once

// `krbn::pressed_keys_manager` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <unordered_set>

namespace krbn {
class pressed_keys_manager {
public:
  void insert(key_code value) {
    insert<key_code>(value);
  }

  void insert(consumer_key_code value) {
    insert<consumer_key_code>(value);
  }

  void insert(pointing_button value) {
    insert<pointing_button>(value);
  }

  void erase(key_code value) {
    erase<key_code>(value);
  }

  void erase(consumer_key_code value) {
    erase<consumer_key_code>(value);
  }

  void erase(pointing_button value) {
    erase<pointing_button>(value);
  }

  bool empty(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return entries_.empty();
  }

private:
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

  typedef boost::variant<key_code,
                         consumer_key_code,
                         pointing_button>
      entry;
  std::unordered_set<entry, boost::hash<entry>> entries_;

  mutable std::mutex mutex_;
};
} // namespace krbn
