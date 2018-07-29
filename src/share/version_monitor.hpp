#pragma once

#include "boost_defs.hpp"

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "gcd_utility.hpp"
#include "logger.hpp"
#include "shared_instance_provider.hpp"
#include <boost/signals2.hpp>
#include <fstream>

namespace krbn {
class version_monitor final : public shared_instance_provider<version_monitor> {
public:
  // Signals

  boost::signals2::signal<void(void)> changed;

  // Methods

  version_monitor(const version_monitor&) = delete;

  version_monitor(void) : started_(false) {
  }

  ~version_monitor(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      file_monitor_ = nullptr;
    });
  }

  void start(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (started_) {
        return;
      }

      started_ = true;

      auto version_file_path = constants::get_version_file_path();
      auto version_file_directory = filesystem::dirname(version_file_path);

      version_ = read_version_file();

      std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
          {version_file_directory, {version_file_path}},
      };
      file_monitor_ = std::make_unique<file_monitor>(targets,
                                                     [this](const std::string&) {
                                                       check_version();
                                                     });
    });
  }

  void manual_check(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      check_version();
    });
  }

private:
  void check_version(void) {
    auto version = read_version_file();
    if (version_ != version) {
      logger::get_logger().info("Version is changed: '{0}' -> '{1}'", version_, version);

      version_ = version;

      changed();
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

  bool started_;
  std::string version_;
  std::unique_ptr<file_monitor> file_monitor_;
};
} // namespace krbn
