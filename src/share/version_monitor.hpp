#pragma once

#include "boost_defs.hpp"

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include "shared_instance_provider.hpp"
#include <boost/signals2.hpp>
#include <fstream>

namespace krbn {
class version_monitor final : public shared_instance_provider<version_monitor> {
public:
  // Signals

  boost::signals2::signal<void(const std::string& version)> changed;

  // Methods

  version_monitor(const version_monitor&) = delete;

  version_monitor(void) {
  }

  ~version_monitor(void) {
    file_monitor_ = nullptr;
  }

  void start(const std::string& version_file_path) {
    std::lock_guard<std::mutex> lock(file_monitor_mutex_);

    if (file_monitor_) {
      return;
    }

    version_file_path_ = version_file_path;
    version_ = read_version_file();

    // Start file_monitor_

    std::vector<std::string> targets = {
        version_file_path_,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this](const std::string&) {
      check_version();
    });

    file_monitor_->start();
  }

  void manual_check(void) {
    file_monitor_->enqueue_file_changed(version_file_path_);
  }

private:
  void check_version(void) {
    auto version = read_version_file();
    if (version_ != version) {
      logger::get_logger().info("Version is changed: '{0}' -> '{1}'", version_, version);

      version_ = version;

      changed(version_);
    }
  }

  std::string read_version_file(void) {
    std::string version;

    std::ifstream stream(version_file_path_);
    if (stream) {
      std::getline(stream, version);
    }

    return version;
  }

  std::string version_file_path_;

  std::unique_ptr<file_monitor> file_monitor_;
  std::mutex file_monitor_mutex_;

  std::string version_;
};
} // namespace krbn
