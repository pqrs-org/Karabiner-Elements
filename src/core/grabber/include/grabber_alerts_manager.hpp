#pragma once

#include "filesystem.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <json/json.hpp>
#include <unordered_set>

namespace krbn {
class grabber_alerts_manager final {
public:
  enum alert {
    system_policy_prevents_loading_kext,
  };

  grabber_alerts_manager(const grabber_alerts_manager&) = delete;

  grabber_alerts_manager(const std::string& output_json_file_path) : output_json_file_path_(output_json_file_path) {
    queue_ = std::make_unique<thread_utility::queue>();
  }

  ~grabber_alerts_manager(void) {
    queue_->terminate();
    queue_ = nullptr;
  }

  static std::string to_string(alert alert) {
#define KRBN_ALERT_TO_STRING(ALERT)          \
  case grabber_alerts_manager::alert::ALERT: \
    return #ALERT;

    switch (alert) {
      KRBN_ALERT_TO_STRING(system_policy_prevents_loading_kext);
    }

#undef KRBN_ALERT_TO_STRING

    return "";
  }

  void async_set_alert(alert alert, bool enabled) {
    queue_->push_back([this, alert, enabled] {
      if (enabled) {
        alerts_.insert(alert);
        logger::get_logger().warn("grabber_alert: {0}", to_string(alert));
      } else {
        alerts_.erase(alert);
      }

      async_save_to_file_();
    });
  }

  void async_save_to_file(void) const {
    queue_->push_back([this] {
      async_save_to_file_();
    });
  }

private:
  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"alerts", alerts_},
    });
  }

  void async_save_to_file_(void) const {
    if (!output_json_file_path_.empty()) {
      json_utility::async_save_to_file(to_json(), output_json_file_path_, 0755, 0644);
    }
  }

  std::unique_ptr<thread_utility::queue> queue_;
  std::string output_json_file_path_;
  std::unordered_set<alert> alerts_;
};

inline void to_json(nlohmann::json& json, const grabber_alerts_manager::alert& alert) {
  json = grabber_alerts_manager::to_string(alert);
}
} // namespace krbn
