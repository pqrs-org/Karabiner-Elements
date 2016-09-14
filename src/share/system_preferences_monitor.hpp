#pragma once

#include "system_preferences.hpp"
#include <chrono>
#include <thread>

class system_preferences_monitor final {
public:
  typedef std::function<void(const system_preferences::values& values)> values_updated_callback;

  system_preferences_monitor(const values_updated_callback& callback) : callback_(callback), exit_loop_(false) {
    if (callback_) {
      callback_(values_);
    }

    thread_ = std::thread([this] { this->worker(); });
  }

  ~system_preferences_monitor(void) {
    exit_loop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

private:
  void worker(void) {
    while (exit_loop_) {
      using namespace std::chrono_literals;

      system_preferences::values v;
      if (values_ != v) {
        values_ = v;
        if (callback_) {
          callback_(values_);
        }
      }

      std::this_thread::sleep_for(1s);
    }
  }

  values_updated_callback callback_;

  system_preferences::values values_;
  std::thread thread_;
  volatile bool exit_loop_;
};
