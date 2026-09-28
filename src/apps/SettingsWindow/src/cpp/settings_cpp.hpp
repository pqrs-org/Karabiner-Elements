#pragma once

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
};
