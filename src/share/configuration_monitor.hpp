#pragma once

#include "boost_defs.hpp"

#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "logger.hpp"

namespace krbn {
class configuration_monitor final {
public:
  typedef std::function<void(std::shared_ptr<core_configuration> core_configuration)> core_configuration_updated_callback;

  configuration_monitor(const std::string& user_core_configuration_file_path,
                        const core_configuration_updated_callback& callback) : user_core_configuration_file_path_(user_core_configuration_file_path),
                                                                               callback_(callback) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_system_configuration_directory(), {constants::get_system_core_configuration_file_path()}},
        {filesystem::dirname(user_core_configuration_file_path), {user_core_configuration_file_path}},
    };

    file_monitor_ = std::make_unique<file_monitor>(targets,
                                                   [this](const std::string&) {
                                                     core_configuration_file_updated_callback();
                                                   });

    // file_monitor doesn't call the callback if target files are not exists.
    // Thus, we call the callback manually at here if the callback is not called yet.
    if (!core_configuration_) {
      core_configuration_file_updated_callback();
    }
  }

  ~configuration_monitor(void) {
    // Release file_monitor_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      file_monitor_ = nullptr;
    });
  }

  std::shared_ptr<core_configuration> get_core_configuration(void) {
    return core_configuration_;
  }

private:
  void core_configuration_file_updated_callback(void) {
    logger::get_logger().info("Load karabiner.json...");

    std::string file_path = constants::get_system_core_configuration_file_path();
    if (filesystem::exists(user_core_configuration_file_path_)) {
      file_path = user_core_configuration_file_path_;
    }

    auto c = std::make_shared<core_configuration>(file_path);
    if (!core_configuration_ || c->is_loaded()) {
      logger::get_logger().info("core_configuration is updated.");
      core_configuration_ = c;
      if (callback_) {
        callback_(core_configuration_);
      }
    }
  }

  std::string user_core_configuration_file_path_;
  core_configuration_updated_callback callback_;

  std::unique_ptr<file_monitor> file_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;
};
} // namespace krbn
