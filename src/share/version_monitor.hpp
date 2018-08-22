#pragma once

// `krbn::version_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/signals2.hpp>
#include <fstream>

namespace krbn {
class version_monitor final {
public:
  // Signals

  boost::signals2::signal<void(const std::string& version)> changed;

  // Methods

  version_monitor(const version_monitor&) = delete;

  version_monitor(const std::string& version_file_path) : version_file_path_(version_file_path) {
    version_ = file_utility::read_file(version_file_path_);

    std::vector<std::string> targets = {
        version_file_path_,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this](auto&& changed_file_path, auto&& weak_changed_file_body) {
      if (auto changed_file_body = weak_changed_file_body.lock()) {
        if (changed_file_body) {
          if (version_) {
            if (*version_ == *changed_file_body) {
              return;
            }
          } else if (!version_ && !changed_file_body) {
            return;
          }

          std::string version_string(std::begin(*changed_file_body),
                                     std::end(*changed_file_body));
          boost::trim(version_string);

          logger::get_logger().info("Version is changed to {0}", version_string);

          version_ = changed_file_body;

          changed(version_string);
        }
      }
    });
  }

  ~version_monitor(void) {
    file_monitor_ = nullptr;
  }

  void async_start() {
    file_monitor_->async_start();
  }

  void async_manual_check(void) {
    file_monitor_->enqueue_file_changed(version_file_path_);
  }

private:
  std::string version_file_path_;

  std::shared_ptr<std::vector<uint8_t>> version_;
  std::unique_ptr<file_monitor> file_monitor_;
};
} // namespace krbn
