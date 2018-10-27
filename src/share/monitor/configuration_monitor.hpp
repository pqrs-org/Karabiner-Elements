#pragma once

// `krbn::configuration_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "logger.hpp"
#include "monitor/file_monitor.hpp"

namespace krbn {
class configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(std::weak_ptr<core_configuration::core_configuration>)> core_configuration_updated;

  // Methods

  configuration_monitor(const std::string& user_core_configuration_file_path,
                        const std::string& system_core_configuration_file_path = constants::get_system_core_configuration_file_path()) : dispatcher_client() {
    std::vector<std::string> targets = {
        user_core_configuration_file_path,
        system_core_configuration_file_path,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this, user_core_configuration_file_path, system_core_configuration_file_path](auto&& changed_file_path,
                                                                                                                       auto&& changed_file_body) {
      auto file_path = changed_file_path;

      if (filesystem::exists(user_core_configuration_file_path)) {
        // Note:
        // user_core_configuration_file_path == system_core_configuration_file_path
        // if console_user_server is not running.

        if (changed_file_path != user_core_configuration_file_path &&
            changed_file_path == system_core_configuration_file_path) {
          // system_core_configuration_file_path is updated.
          // We ignore it because we are using user_core_configuration_file_path.
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

      auto c = std::make_shared<core_configuration::core_configuration>(file_path);

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

      enqueue_to_dispatcher([this, c] {
        core_configuration_updated(c);
      });
    });
  }

  virtual ~configuration_monitor(void) {
    detach_from_dispatcher([this] {
      file_monitor_ = nullptr;
    });
  }

  void async_start() {
    file_monitor_->async_start();
  }

  std::shared_ptr<core_configuration::core_configuration> get_core_configuration(void) const {
    std::lock_guard<std::mutex> lock(core_configuration_mutex_);

    return core_configuration_;
  }

private:
  std::unique_ptr<file_monitor> file_monitor_;

  std::shared_ptr<core_configuration::core_configuration> core_configuration_;
  mutable std::mutex core_configuration_mutex_;
};
} // namespace krbn
