#pragma once

#include "json_utility.hpp"
#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

class settings_components_manager;
extern std::weak_ptr<settings_components_manager> settings_components_manager_;
extern std::mutex settings_components_manager_mutex_;

class settings_cpp final {
public:
  [[nodiscard]] static std::shared_ptr<settings_components_manager> get_components_manager() {
    std::lock_guard<std::mutex> lock(settings_components_manager_mutex_);
    return settings_components_manager_.lock();
  }

  static void set_components_manager(std::weak_ptr<settings_components_manager> manager) {
    std::lock_guard<std::mutex> lock(settings_components_manager_mutex_);
    settings_components_manager_ = std::move(manager);
  }

  [[nodiscard]] static krbn::device_identifiers make_device_identifiers(const char* device_identifiers_json) {
    if (device_identifiers_json) {
      try {
        return krbn::json_utility::parse_jsonc(device_identifiers_json).get<krbn::device_identifiers>();
      } catch (const std::exception& e) {
        std::cerr << __func__ << ": " << e.what() << std::endl;
      }
    }

    return krbn::device_identifiers();
  }
};
