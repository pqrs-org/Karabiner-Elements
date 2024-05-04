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
  notification_message_manager(const std::filesystem::path& notification_message_file_path)
      : dispatcher_client(),
        notification_message_file_path_(notification_message_file_path) {
    enqueue_to_dispatcher([this] {
      save_message("");
    });
  }

  virtual ~notification_message_manager(void) {
    detach_from_dispatcher();
  }

  void async_set_device_ungrabbable_temporarily_message(device_id id,
                                                        const std::string& message) {
    enqueue_to_dispatcher([this, id, message] {
      device_ungrabbable_temporarily_messages_[id] = message;

      save_message_if_needed();
    });
  }

  void async_erase_device(device_id id) {
    enqueue_to_dispatcher([this, id] {
      device_ungrabbable_temporarily_messages_.erase(id);

      save_message_if_needed();
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
      save_message_if_needed();
    });
  }

  void async_clear_sticky_modifiers_message(void) {
    enqueue_to_dispatcher([this] {
      messages_["__system__sticky_modifiers"] = "";

      save_message_if_needed();
    });
  }

  void async_set_notification_message(const notification_message& notification_message) {
    enqueue_to_dispatcher([this, notification_message] {
      messages_[fmt::format("__user__{0}", notification_message.get_id())] = notification_message.get_text();

      save_message_if_needed();
    });
  }

private:
  void save_message(const std::string& message) const {
    json_writer::async_save_to_file(
        nlohmann::json::object({
            {"body", message},
        }),
        notification_message_file_path_.string(),
        0755,
        0644);
  }

  void save_message_if_needed(void) {
    auto message = make_message();
    if (previous_message_ != message) {
      previous_message_ = message;
      save_message(message);
    }
  }

  std::string make_message(void) const {
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

    return ss.str();
  }

  std::filesystem::path notification_message_file_path_;
  std::map<device_id, std::string> device_ungrabbable_temporarily_messages_;
  std::map<std::string, std::string> messages_;
  std::string previous_message_;
};
} // namespace krbn
