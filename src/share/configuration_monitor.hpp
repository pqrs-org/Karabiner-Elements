#pragma once

#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "logger.hpp"

namespace krbn {
class configuration_monitor final {
public:
  // Signals

  boost::signals2::signal<void(std::shared_ptr<core_configuration> core_configuration)>
      core_configuration_updated;

  // Methods

  configuration_monitor(const std::string& user_core_configuration_file_path,
                        const std::string& system_core_configuration_file_path = constants::get_system_core_configuration_file_path()) : user_core_configuration_file_path_(user_core_configuration_file_path),
                                                                                                                                         system_core_configuration_file_path_(system_core_configuration_file_path) {
  }

  ~configuration_monitor(void) {
    file_monitor_ = nullptr;
  }

  void start() {
    std::lock_guard<std::mutex> lock(file_monitor_mutex_);

    std::vector<std::string> targets = {
        user_core_configuration_file_path_,
        system_core_configuration_file_path_,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this](auto&& file_path) {
      core_configuration_file_changed();
    });

    file_monitor_->start();

    // `core_configuration_file_changed` is enqueued by `file_monitor::start` only if the file exists.
    // We have to enqueue it manually for when the file does not exist.

    file_monitor_->get_run_loop_thread()->enqueue(^{
      {
        std::lock_guard<std::mutex> lock(core_configuration_mutex_);

        if (core_configuration_) {
          // `core_configuration_file_changed` is already called.
          // We do not need call the method again.
          return;
        }
      }

      core_configuration_file_changed();
    });
  }

  std::shared_ptr<core_configuration> get_core_configuration(void) const {
    std::lock_guard<std::mutex> lock(core_configuration_mutex_);

    return core_configuration_;
  }

private:
  void core_configuration_file_changed(void) {
    std::string file_path = system_core_configuration_file_path_;
    if (filesystem::exists(user_core_configuration_file_path_)) {
      file_path = user_core_configuration_file_path_;
    }

    logger::get_logger().info("Load {0}...", file_path);

    auto c = std::make_shared<core_configuration>(file_path);

    {
      std::lock_guard<std::mutex> lock(core_configuration_mutex_);

      if (core_configuration_ && !c->is_loaded()) {
        return;
      }

      if (core_configuration_ && core_configuration_->to_json() == c->to_json()) {
        return;
      }

      logger::get_logger().info("core_configuration is updated.");
      core_configuration_ = c;
    }

    core_configuration_updated(c);
  }

  std::string user_core_configuration_file_path_;
  std::string system_core_configuration_file_path_;

  std::unique_ptr<file_monitor> file_monitor_;
  mutable std::mutex file_monitor_mutex_;

  std::shared_ptr<core_configuration> core_configuration_;
  mutable std::mutex core_configuration_mutex_;
};
} // namespace krbn
