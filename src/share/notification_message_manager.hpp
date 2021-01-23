#pragma once

#include "modifier_flag_manager.hpp"
#include "types/device_id.hpp"
#include <map>
#include <sstream>

namespace krbn {
class notification_message_manager final {
public:
  notification_message_manager(std::weak_ptr<console_user_server_client> weak_console_user_server_client) : weak_console_user_server_client_(weak_console_user_server_client) {
    post_message("");
  }

  void set_device_ungrabbable_temporarily_message(device_id id,
                                                  const std::string& message) {
    device_ungrabbable_temporarily_messages_[id] = message;

    post_message_if_needed();
  }

  void erase_device(device_id id) {
    device_ungrabbable_temporarily_messages_.erase(id);

    post_message_if_needed();
  }

  void update_sticky_modifiers_message(const modifier_flag_manager& modifier_flag_manager) {
    std::stringstream ss;

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
      if (modifier_flag_manager.is_sticky_active(f)) {
        if (auto name = get_modifier_flag_name(f)) {
          ss << "sticky " << *name << "\n";
        }
      }
    }

    sticky_modifiers_message_ = ss.str();

    post_message_if_needed();
  }

private:
  void post_message(const std::string& message) const {
    if (auto c = weak_console_user_server_client_.lock()) {
      c->async_set_notification_message(message);
    }
  }

  void post_message_if_needed(void) {
    auto message = make_message();
    if (previous_message_ != message) {
      previous_message_ = message;
      post_message(message);
    }
  }

  std::string make_message(void) const {
    std::stringstream ss;

    for (const auto& m : device_ungrabbable_temporarily_messages_) {
      if (!m.second.empty()) {
        ss << m.second;
        break;
      }
    }

    if (!sticky_modifiers_message_.empty()) {
      if (ss.tellp() > 0) {
        ss << "\n";
      }
      ss << sticky_modifiers_message_;
    }

    return ss.str();
  }

  std::weak_ptr<console_user_server_client> weak_console_user_server_client_;
  std::map<device_id, std::string> device_ungrabbable_temporarily_messages_;
  std::string sticky_modifiers_message_;
  std::string previous_message_;
};
} // namespace krbn
