#pragma once

// `krbn::configuration_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "core_configuration.hpp"
#include "logger.hpp"
#include "monitor/file_monitor.hpp"

namespace krbn {
class configuration_monitor final {
public:
  // Signals

  boost::signals2::signal<void(std::weak_ptr<const core_configuration>)> core_configuration_updated;

  // Methods

  configuration_monitor(const std::string& user_core_configuration_file_path,
                        const std::string& system_core_configuration_file_path = constants::get_system_core_configuration_file_path()) {
    std::vector<std::string> targets = {
        user_core_configuration_file_path,
        system_core_configuration_file_path,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this, user_core_configuration_file_path, system_core_configuration_file_path](auto&& changed_file_path,
                                                                                                                       auto&& changed_file_body) {
      auto file_path = changed_file_path;

      if (filesystem::exists(user_core_configuration_file_path)) {
        if (changed_file_path == system_core_configuration_file_path) {
          return;
        }
      } else {
        if (changed_file_path == user_core_configuration_file_path) {
          // user_core_configuration_file_path is removed.

          if (filesystem::exists(system_core_configuration_file_path)) {
            file_path = system_core_configuration_file_path;
          }
        }
      }

      if (filesystem::exists(file_path)) {
        logger::get_logger().info("Load {0}...", file_path);
      }

      auto c = std::make_shared<core_configuration>(file_path);

      if (core_configuration_ && !c->is_loaded()) {
        return;
      }

      if (core_configuration_ && core_configuration_->to_json() == c->to_json()) {
        return;
      }

      {
        std::lock_guard<std::mutex> lock(core_configuration_mutex_);

        core_configuration_ = c;
      }

      logger::get_logger().info("core_configuration is updated.");

      core_configuration_updated(c);
    });
  }

  ~configuration_monitor(void) {
    file_monitor_ = nullptr;
  }

  void async_start() {
    file_monitor_->async_start();
  }

  std::shared_ptr<const core_configuration> get_core_configuration(void) const {
    std::lock_guard<std::mutex> lock(core_configuration_mutex_);

    return core_configuration_;
  }

  std::shared_ptr<cf_utility::run_loop_thread> get_run_loop_thread(void) const {
    return file_monitor_->get_run_loop_thread();
  }

private:
  std::unique_ptr<file_monitor> file_monitor_;

  std::shared_ptr<const core_configuration> core_configuration_;
  mutable std::mutex core_configuration_mutex_;
};
} // namespace krbn
