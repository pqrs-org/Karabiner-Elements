#pragma once

#include "boost_defs.hpp"

#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include <spdlog/spdlog.h>

class configuration_monitor final {
public:
  typedef std::function<void(core_configuration& core_configuration)> core_configuration_updated_callback;

  configuration_monitor(spdlog::logger& logger,
                        const std::string& user_core_configuration_file_path,
                        const core_configuration_updated_callback& callback) : logger_(logger),
                                                                               user_core_configuration_file_path_(user_core_configuration_file_path),
                                                                               callback_(callback) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {filesystem::dirname(user_core_configuration_file_path), {user_core_configuration_file_path}},
    };

    file_monitor_ = std::make_unique<file_monitor>(logger_,
                                                   targets,
                                                   [this](const std::string& file_path) {
                                                     logger_.info("{0} is updated.", file_path);
                                                     core_configuration_file_updated_callback(file_path);
                                                   });

    // file_monitor doesn't call the callback until target files are actually updated.
    // Thus, we call the callback manually at here.
    core_configuration_file_updated_callback(user_core_configuration_file_path);
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
  void core_configuration_file_updated_callback(const std::string& file_path) {
    auto c = std::make_shared<core_configuration>(logger_, user_core_configuration_file_path_);
    if (!core_configuration_ || c->is_loaded()) {
      logger_.info("core_configuration is updated.", file_path);
      core_configuration_ = c;
      if (callback_) {
        callback_(*core_configuration_);
      }
    }
  }

  spdlog::logger& logger_;
  std::string user_core_configuration_file_path_;
  core_configuration_updated_callback callback_;

  std::unique_ptr<file_monitor> file_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;
};
