#pragma once

#include "boost_defs.hpp"

#include "connected_devices.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include <spdlog/spdlog.h>

namespace krbn {
class connected_devices_monitor final {
public:
  typedef std::function<void(std::shared_ptr<connected_devices> connected_devices)> connected_devices_updated_callback;

  connected_devices_monitor(spdlog::logger& logger,
                            const connected_devices_updated_callback& callback) : logger_(logger),
                                                                                  callback_(callback) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_tmp_directory(), {constants::get_devices_json_file_path()}},
    };

    file_monitor_ = std::make_unique<file_monitor>(logger_,
                                                   targets,
                                                   [this](const std::string&) {
                                                     call_connected_devices_updated_callback();
                                                   });

    // file_monitor doesn't call the callback if target files are not exists.
    // Thus, we call the callback manually at here if the callback is not called yet.
    if (!connected_devices_) {
      call_connected_devices_updated_callback();
    }
  }

  ~connected_devices_monitor(void) {
    // Release file_monitor_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      file_monitor_ = nullptr;
    });
  }

  std::shared_ptr<connected_devices> get_connected_devices(void) {
    return connected_devices_;
  }

private:
  void call_connected_devices_updated_callback(void) {
    logger_.info("Load karabiner_grabber_devices.json...");

    auto c = std::make_shared<connected_devices>(logger_, constants::get_devices_json_file_path());
    if (!connected_devices_ || c->is_loaded()) {
      logger_.info("connected_devices are updated.");
      connected_devices_ = c;
      if (callback_) {
        callback_(connected_devices_);
      }
    }
  }

  spdlog::logger& logger_;
  connected_devices_updated_callback callback_;

  std::unique_ptr<file_monitor> file_monitor_;
  std::shared_ptr<connected_devices> connected_devices_;
};
}
