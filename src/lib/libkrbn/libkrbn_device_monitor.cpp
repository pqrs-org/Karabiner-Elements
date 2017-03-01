#include "constants.hpp"
#include "file_monitor.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"

namespace {
class libkrbn_device_monitor_class final {
public:
  libkrbn_device_monitor_class(const libkrbn_device_monitor_class&) = delete;

  libkrbn_device_monitor_class(libkrbn_device_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_tmp_directory(), {constants::get_devices_json_file_path()}},
    };

    file_monitor_ = std::make_unique<file_monitor>(libkrbn::get_logger(),
                                                   targets,
                                                   [this](const std::string& file_path) { cpp_callback(file_path); });

    cpp_callback(constants::get_devices_json_file_path());
  }

private:
  void cpp_callback(const std::string& file_path) {
    if (callback_) {
      callback_(refcon_);
    }
  }

  libkrbn_device_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<file_monitor> file_monitor_;
};
}

bool libkrbn_device_monitor_initialize(libkrbn_device_monitor** out, libkrbn_device_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_device_monitor*>(new libkrbn_device_monitor_class(callback, refcon));
  return true;
}

void libkrbn_device_monitor_terminate(libkrbn_device_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_device_monitor_class*>(*p);
    *p = nullptr;
  }
}
