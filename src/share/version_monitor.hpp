#pragma once

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "gcd_utility.hpp"
#include <fstream>

namespace krbn {
class version_monitor final {
public:
  typedef std::function<void(void)> callback;

  version_monitor(const version_monitor&) = delete;

  version_monitor(spdlog::logger& logger,
                  const callback& callback) : logger_(logger),
                                              callback_(callback) {
    auto version_file_path = constants::get_version_file_path();
    auto version_file_directory = filesystem::dirname(version_file_path);

    version_ = read_version_file();

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {version_file_directory, {version_file_path}},
    };
    file_monitor_ = std::make_unique<file_monitor>(logger_, targets, std::bind(&version_monitor::version_file_updated_callback, this, std::placeholders::_1));
  }

  ~version_monitor(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      file_monitor_ = nullptr;
    });
  }

  void manual_check(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      logger_.info("Check version...");
      check_version();
    });
  }

private:
  void version_file_updated_callback(const std::string& file_path) {
    logger_.info("Version file is updated.");
    check_version();
  }

  void check_version(void) {
    auto version = read_version_file();
    if (version_ != version) {
      logger_.info("Version is changed: '{0}' -> '{1}'", version_, version);
      if (callback_) {
        callback_();
      }
      version_ = version;
    }
  }

  std::string read_version_file(void) {
    std::string version;

    std::ifstream stream(constants::get_version_file_path());
    if (stream) {
      std::getline(stream, version);
    }

    return version;
  }

  spdlog::logger& logger_;
  callback callback_;

  std::string version_;
  std::unique_ptr<file_monitor> file_monitor_;
};
}
