#pragma once

#include "types/device_id.hpp"
#include <map>

namespace krbn {
namespace device_grabber_details {
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
    for (const auto& m : device_ungrabbable_temporarily_messages_) {
      if (!m.second.empty()) {
        return m.second;
      }
    }
    return "";
  }

  std::weak_ptr<console_user_server_client> weak_console_user_server_client_;
  std::map<device_id, std::string> device_ungrabbable_temporarily_messages_;
  std::string previous_message_;
};
} // namespace device_grabber_details
} // namespace krbn
