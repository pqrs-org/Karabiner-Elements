#pragma once

// `krbn::grabber_alerts_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include <fstream>

namespace krbn {
class grabber_alerts_monitor final {
public:
  // Signals

  boost::signals2::signal<void(const nlohmann::json& alerts)> alerts_changed;

  // Methods

  grabber_alerts_monitor(const grabber_alerts_monitor&) = delete;

  grabber_alerts_monitor(const std::string& grabber_alerts_json_file_path) {
    std::vector<std::string> targets = {
        grabber_alerts_json_file_path,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this](const std::string& file_path) {
      std::ifstream stream(file_path);
      if (stream) {
        try {
          auto json = nlohmann::json::parse(stream);

          // json example
          //
          // {
          //     "alerts": [
          //         "system_policy_prevents_loading_kext"
          //     ]
          // }

          if (auto v = json_utility::find_array(json, "alerts")) {
            auto s = v->dump();
            if (last_json_string_ != s) {
              last_json_string_ = s;
              alerts_changed(*v);
            }
          }
        } catch (std::exception& e) {
          logger::get_logger().error("parse error in {0}: {1}", file_path, e.what());
        }
      }
    });
  }

  void start(void) {
    file_monitor_->start();
  }

private:
  std::unique_ptr<file_monitor> file_monitor_;
  boost::optional<std::string> last_json_string_;
};
} // namespace krbn
