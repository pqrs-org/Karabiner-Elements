#pragma once

#include "json_writer.hpp"
#include "modifier_flag_manager.hpp"
#include "types/device_id.hpp"
#include <map>
#include <pqrs/dispatcher.hpp>
#include <sstream>

// `krbn::notification_message_manager` can be used safely in a multi-threaded environment.

namespace krbn {
class notification_message_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  notification_message_manager(void)
      : dispatcher_client() {
  }

  virtual ~notification_message_manager(void) {
    detach_from_dispatcher();
  }

  const std::string& get_full_message(void) const {
    return full_message_;
  }

  void async_set_device_ungrabbable_temporarily_message(device_id id,
                                                        const std::string& message) {
    enqueue_to_dispatcher([this, id, message] {
      device_ungrabbable_temporarily_messages_[id] = message;

      update_full_message();
    });
  }

  void async_erase_device(device_id id) {
    enqueue_to_dispatcher([this, id] {
      device_ungrabbable_temporarily_messages_.erase(id);

      update_full_message();
    });
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
            messages_[id] = fmt::format("sticky {0}", *name);
          });
        } else {
          enqueue_to_dispatcher([this, id] {
            messages_[id] = "";
          });
        }
      }
    }

    enqueue_to_dispatcher([this] {
      update_full_message();
    });
  }

  void async_clear_sticky_modifiers_message(void) {
    enqueue_to_dispatcher([this] {
      messages_["__system__sticky_modifiers"] = "";

      update_full_message();
    });
  }

  void async_set_notification_message(const notification_message& notification_message) {
    enqueue_to_dispatcher([this, notification_message] {
      messages_[fmt::format("__user__{0}", notification_message.get_id())] = notification_message.get_text();

      update_full_message();
    });
  }

private:
  void update_full_message(void) {
    std::stringstream ss;

    for (const auto& m : device_ungrabbable_temporarily_messages_) {
      if (!m.second.empty()) {
        if (ss.tellp() > 0) {
          ss << "\n";
        }
        ss << m.second;
        break;
      }
    }

    for (const auto& m : messages_) {
      if (!m.second.empty()) {
        if (ss.tellp() > 0) {
          ss << "\n";
        }
        ss << m.second;
      }
    }

    full_message_ = ss.str();
  }

  std::map<device_id, std::string> device_ungrabbable_temporarily_messages_;
  std::map<std::string, std::string> messages_;
  std::string full_message_;
};
} // namespace krbn
