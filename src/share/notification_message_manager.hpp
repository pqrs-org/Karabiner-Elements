#pragma once

#include "json_writer.hpp"
#include "modifier_flag_manager.hpp"
#include "types/device_id.hpp"
#include <functional>
#include <map>
#include <memory>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <sstream>
#include <utility>

// `krbn::notification_message_manager` can be used safely in a multi-threaded environment.

namespace krbn {
class notification_message_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(const std::string&)> notification_message_changed;

  explicit notification_message_manager(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher =
                                            pqrs::dispatcher::extra::get_shared_dispatcher())
      : dispatcher_client(std::move(weak_dispatcher)) {
  }

  ~notification_message_manager() override {
    detach_from_dispatcher([this] {
      messages_.clear();
    });
  }

  [[nodiscard]] const std::string& get_full_message() const {
    return full_message_;
  }

  void async_update_sticky_modifiers_message(const modifier_flag_manager& modifier_flag_manager) {
    for (const auto& f : {
             modifier_flag::left_control,
             modifier_flag::left_shift,
             modifier_flag::left_option,
             modifier_flag::left_command,
             modifier_flag::right_control,
             modifier_flag::right_shift,
             modifier_flag::right_option,
             modifier_flag::right_command,
             modifier_flag::fn,
         }) {
      if (auto name = get_modifier_flag_name(f)) {
        auto id = fmt::format("__system__sticky_{0}", *name);

        if (modifier_flag_manager.is_sticky_active(f)) {
          enqueue_to_dispatcher([this, id, name] {
            messages_[id].set_message(fmt::format("sticky {0}", *name));
          });
        } else {
          enqueue_to_dispatcher([this, id] {
            messages_[id].set_message("");
          });
        }
      }
    }

    enqueue_to_dispatcher([this] {
      update_full_message();
    });
  }

  void async_clear_sticky_modifiers_message() {
    enqueue_to_dispatcher([this] {
      for (const auto& f : {
               modifier_flag::left_control,
               modifier_flag::left_shift,
               modifier_flag::left_option,
               modifier_flag::left_command,
               modifier_flag::right_control,
               modifier_flag::right_shift,
               modifier_flag::right_option,
               modifier_flag::right_command,
               modifier_flag::fn,
           }) {
        if (auto name = get_modifier_flag_name(f)) {
          auto id = fmt::format("__system__sticky_{0}", *name);
          messages_[id].set_message("");
        }
      }

      update_full_message();
    });
  }

  void async_set_notification_message(const notification_message& notification_message) {
    enqueue_to_dispatcher([this, notification_message] {
      auto id = fmt::format("__user__{0}", notification_message.get_id());
      auto& entry = messages_[id];

      entry.set_message(notification_message.get_text());

      auto duration_milliseconds = notification_message.get_duration_milliseconds();
      if (!notification_message.get_text().empty() &&
          duration_milliseconds.count() > 0) {
        entry.debounce_expiration(
            *this,
            [this, id] {
              messages_[id].set_message("");
              update_full_message();
            },
            duration_milliseconds);
      } else {
        entry.cancel_expiration();
      }

      update_full_message();
    });
  }

private:
  class message_entry final {
  public:
    ~message_entry() {
      cancel_expiration();
    }

    [[nodiscard]] const std::string& get_message() const {
      return message_;
    }

    void set_message(std::string value) {
      message_ = std::move(value);
    }

    void cancel_expiration() {
      if (expiration_task_) {
        expiration_task_->cancel();
      }
    }

    void debounce_expiration(pqrs::dispatcher::extra::dispatcher_client& dispatcher_client,
                             std::function<void()> function,
                             pqrs::dispatcher::duration delay) {
      if (!expiration_task_) {
        expiration_task_ = std::make_unique<pqrs::dispatcher::extra::debounced_task>(dispatcher_client);
      }

      expiration_task_->debounce_after(std::move(function), delay);
    }

  private:
    std::string message_;
    std::unique_ptr<pqrs::dispatcher::extra::debounced_task> expiration_task_;
  };

  void update_full_message() {
    std::stringstream ss;

    for (const auto& m : messages_) {
      if (!m.second.get_message().empty()) {
        if (ss.tellp() > 0) {
          ss << "\n";
        }
        ss << m.second.get_message();
      }
    }

    auto full_message = ss.str();
    if (full_message_ != full_message) {
      full_message_ = std::move(full_message);
      notification_message_changed(full_message_);
    }
  }

  std::map<std::string, message_entry> messages_;
  std::string full_message_;
};
} // namespace krbn
