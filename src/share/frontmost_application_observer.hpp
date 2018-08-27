#pragma once

// `krbn::frontmost_application_observer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "frontmost_application_observer_objc.h"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class frontmost_application_observer final {
public:
  // Signals

  boost::signals2::signal<void(const std::string& bundle_identifier, const std::string& file_path)> frontmost_application_changed;

  // Methods

  frontmost_application_observer(void) : observer_(nullptr) {
    queue_ = std::make_unique<thread_utility::queue>();
  }

  ~frontmost_application_observer(void) {
    async_stop();

    queue_->terminate();
    queue_ = nullptr;
  }

  void async_start(void) {
    queue_->push_back([this] {
      if (observer_) {
        logger::get_logger().warn("frontmost_application_observer is already started.");
        return;
      }

      krbn_frontmost_application_observer_initialize(&observer_,
                                                     static_cpp_callback,
                                                     this);

      logger::get_logger().info("frontmost_application_observer is started.");
    });
  }

  void async_stop(void) {
    queue_->push_back([this] {
      if (!observer_) {
        return;
      }

      krbn_frontmost_application_observer_terminate(&observer_);
      observer_ = nullptr;

      logger::get_logger().info("frontmost_application_observer is stopped.");
    });
  }

private:
  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* file_path,
                                  void* context) {
    auto observer = reinterpret_cast<frontmost_application_observer*>(context);
    if (observer) {
      std::string bundle_identifier_string;
      if (bundle_identifier) {
        bundle_identifier_string = bundle_identifier;
      }

      std::string file_path_string;
      if (file_path) {
        file_path_string = file_path;
      }

      observer->cpp_callback(bundle_identifier_string, file_path_string);
    }
  }

  void cpp_callback(const std::string& bundle_identifier,
                    const std::string& file_path) {
    queue_->push_back([this, bundle_identifier, file_path] {
      frontmost_application_changed(bundle_identifier, file_path);
    });
  }

  std::unique_ptr<thread_utility::queue> queue_;
  krbn_frontmost_application_observer_objc* observer_;
};
} // namespace krbn
