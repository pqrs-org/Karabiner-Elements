#pragma once

#include "filesystem.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include <json/json.hpp>
#include <unordered_set>

namespace krbn {
class grabber_alerts_manager final {
public:
  enum alert {
    system_policy_prevents_loading_kext,
  };

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

  static void set_alert(alert alert, bool enabled) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    auto& manager = instance();

    if (enabled) {
      manager.alerts_.insert(alert);
      logger::get_logger().warn("grabber_alert: {0}", to_string(alert));
    } else {
      manager.alerts_.erase(alert);
    }

    manager.async_save_to_file_();
  }

  static void enable_json_output(const std::string& output_json_file_path) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    auto& manager = instance();

    manager.output_json_file_path_ = output_json_file_path;
  }

  static void disable_json_output(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    auto& manager = instance();

    manager.output_json_file_path_.clear();
  }

  static void async_save_to_file(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    auto& manager = instance();

    manager.async_save_to_file_();
  }

private:
  static grabber_alerts_manager& instance(void) {
    static bool once = false;
    static std::unique_ptr<grabber_alerts_manager> manager;

    if (!once) {
      once = true;
      manager = std::make_unique<grabber_alerts_manager>();
    }

    return *manager;
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"alerts", alerts_},
    });
  }

  void async_save_to_file_(void) {
    if (!output_json_file_path_.empty()) {
      json_utility::async_save_to_file(to_json(), output_json_file_path_, 0755, 0644);
    }
  }

  std::string output_json_file_path_;
  std::unordered_set<alert> alerts_;
};

inline void to_json(nlohmann::json& json, const grabber_alerts_manager::alert& alert) {
  json = grabber_alerts_manager::to_string(alert);
}
} // namespace krbn
