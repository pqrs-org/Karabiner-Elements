#pragma once

#include <functional>
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <string>

namespace krbn::console_user_server {
class ui_bridge final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(size_t)> profile_selection_requested;

  using string_callback = void (*)(const char*);

  ui_bridge()
      : dispatcher_client() {
  }

  ~ui_bridge() override {
    detach_from_dispatcher();
  }

  void register_ui_state_callback(string_callback callback) {
    std::lock_guard<std::mutex> lock(mutex_);

    ui_state_callback_ = callback;
    if (callback && !ui_state_.empty()) {
      callback(ui_state_.c_str());
    }
  }

  void register_notification_message_callback(string_callback callback) {
    std::lock_guard<std::mutex> lock(mutex_);

    notification_message_callback_ = callback;
    if (callback) {
      callback(notification_message_.c_str());
    }
  }

  void set_ui_state(std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);

    ui_state_ = std::move(value);
    if (ui_state_callback_) {
      ui_state_callback_(ui_state_.c_str());
    }
  }

  void set_notification_message(std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (notification_message_ == value) {
      return;
    }

    notification_message_ = std::move(value);
    if (notification_message_callback_) {
      notification_message_callback_(notification_message_.c_str());
    }
  }

  void select_profile(size_t index) {
    enqueue_to_dispatcher([this, index] {
      profile_selection_requested(index);
    });
  }

private:
  std::mutex mutex_;
  std::string ui_state_;
  std::string notification_message_;
  string_callback ui_state_callback_ = nullptr;
  string_callback notification_message_callback_ = nullptr;
};
} // namespace krbn::console_user_server
