#pragma once

// `krbn::grabber_alerts_manager` can be used safely in a multi-threaded environment.

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

  void set_alert(alert alert, bool enabled) {
    {
      std::lock_guard<std::mutex> lock(alerts_mutex_);

      if (enabled) {
        alerts_.insert(alert);
        logger::get_logger().warn("grabber_alert: {0}", to_string(alert));
      } else {
        alerts_.erase(alert);
      }
    }

    async_save_to_file();
  }

  void async_save_to_file(void) const {
    if (!output_json_file_path_.empty()) {
      json_utility::async_save_to_file(to_json(), output_json_file_path_, 0755, 0644);
    }
  }

  nlohmann::json to_json(void) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);

    return nlohmann::json({
        {"alerts", alerts_},
    });
  }

private:
  std::string output_json_file_path_;

  std::unordered_set<alert> alerts_;
  mutable std::mutex alerts_mutex_;
};

inline void to_json(nlohmann::json& json, const grabber_alerts_manager::alert& alert) {
  json = grabber_alerts_manager::to_string(alert);
}
} // namespace krbn
