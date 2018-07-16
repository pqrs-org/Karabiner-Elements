#pragma once

#include <memory>
#include <mutex>

namespace krbn {
template <typename T>
class shared_instance_provider {
public:
  static std::shared_ptr<T> get_instance(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::shared_ptr<T> instance;
    if (!instance) {
      instance = std::make_shared<T>();
    }

    return instance;
  }
};
} // namespace krbn
