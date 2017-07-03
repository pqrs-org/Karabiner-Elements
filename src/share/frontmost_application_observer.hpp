#pragma once

#include "frontmost_application_observer_objc.h"
#include "logger.hpp"

namespace krbn {
class frontmost_application_observer final {
public:
  typedef std::function<void(const std::string& bundle_identifier,
                             const std::string& file_path)>
      callback;

  frontmost_application_observer(const callback& callback) : callback_(callback),
                                                             observer_(nullptr) {
    krbn_frontmost_application_observer_initialize(&observer_,
                                                   static_cpp_callback,
                                                   this);
  }

  ~frontmost_application_observer(void) {
    krbn_frontmost_application_observer_terminate(&observer_);
  }

private:
  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* file_path,
                                  void* context) {
    auto observer = reinterpret_cast<frontmost_application_observer*>(context);
    if (observer) {
      observer->cpp_callback(bundle_identifier, file_path);
    }
  }

  void cpp_callback(const std::string& bundle_identifier,
                    const std::string& file_path) {
    if (callback_) {
      callback_(bundle_identifier, file_path);
    }
  }

  callback callback_;
  krbn_frontmost_application_observer_objc* observer_;
};
} // namespace krbn
