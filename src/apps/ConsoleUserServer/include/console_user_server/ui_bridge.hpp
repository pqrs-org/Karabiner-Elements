#pragma once

#include "dispatcher_client_constructor_guard.hpp"
#include <functional>
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <string>

namespace krbn::console_user_server {
class ui_bridge final : public pqrs::dispatcher::extra::dispatcher_client {
  krbn::dispatcher_client_constructor_guard dispatcher_client_constructor_guard_{*this};

public:
  nod::signal<void(size_t)> profile_selection_requested;
  nod::signal<void(const std::string&)> resolved_ui_language_changed;

  using string_callback = void (*)(const char*);

  ui_bridge()
      : dispatcher_client() {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~ui_bridge() override {
    unregister_callbacks_and_detach();
  }

  void unregister_callbacks_and_detach() {
    std::call_once(unregister_callbacks_and_detach_once_, [this] {
      detach_from_dispatcher([this] {
        profile_selection_requested.disconnect_all_slots();
        resolved_ui_language_changed.disconnect_all_slots();

        std::lock_guard<std::mutex> lock(mutex_);
        ui_state_callback_ = nullptr;
        notification_message_callback_ = nullptr;
      });
    });
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

  void async_set_resolved_ui_language(std::string language) {
    enqueue_to_dispatcher([this, language = std::move(language)] {
      if (language.empty() || language == "auto") {
        return;
      }
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resolved_ui_language_ == language) {
          return;
        }
        resolved_ui_language_ = language;
      }
      resolved_ui_language_changed(language);
    });
  }

  [[nodiscard]] std::string get_resolved_ui_language() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return resolved_ui_language_;
  }

private:
  mutable std::mutex mutex_;
  std::string resolved_ui_language_ = "en";
  std::string ui_state_;
  std::string notification_message_;
  string_callback ui_state_callback_ = nullptr;
  string_callback notification_message_callback_ = nullptr;
  std::once_flag unregister_callbacks_and_detach_once_;
};
} // namespace krbn::console_user_server
