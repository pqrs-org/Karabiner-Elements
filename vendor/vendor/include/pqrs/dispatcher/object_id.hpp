#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::object_id` can be used safely in a multi-threaded environment.

#include <cstdint>
#include <limits>
#include <mutex>
#include <ostream>
#include <unordered_set>

namespace pqrs::dispatcher {
class object_id final {
public:
  object_id(const object_id&) = delete;
  object_id(object_id&&) = default;

  ~object_id() {
    manager::erase(value_);
  }

  static object_id make_new_object_id() {
    return object_id(manager::make());
  }

  static size_t active_object_id_count() {
    return manager::size();
  }

  uint64_t get() const {
    return value_;
  }

private:
  class manager final {
  public:
    static uint64_t make() {
      std::lock_guard<std::mutex> lock(mutex());

      if (set().size() >= std::numeric_limits<uint64_t>::max()) {
        throw std::runtime_error("pqrs::dispatcher::object_id::manager::make_new_object_id fails to allocate new object_id.");
      }

      while (true) {
        auto value = ++(last_value());
        if (!set().contains(value)) {
          set().insert(value);
          last_value() = value;
          return value;
        }
      }
    }

    static void erase(uint64_t value) {
      std::lock_guard<std::mutex> lock(mutex());

      set().erase(value);
    }

    static size_t size() {
      std::lock_guard<std::mutex> lock(mutex());

      return set().size();
    }

  private:
    static std::mutex& mutex() {
      static std::mutex mutex;
      return mutex;
    }

    static std::unordered_set<uint64_t>& set() {
      static std::unordered_set<uint64_t> set;
      return set;
    }

    static uint64_t& last_value() {
      static uint64_t value = 0;
      return value;
    }
  };

  object_id(uint64_t value) : value_(value) {
  }

  uint64_t value_;
};

inline object_id make_new_object_id() {
  return object_id::make_new_object_id();
}

inline size_t active_object_id_count() {
  return object_id::active_object_id_count();
}

inline std::ostream& operator<<(std::ostream& stream, const object_id& value) {
  stream << value.get();
  return stream;
}
} // namespace pqrs::dispatcher
