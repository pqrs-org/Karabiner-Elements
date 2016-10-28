#include "constants.hpp"
#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"

namespace {
class libkrbn_configuration_monitor_class {
public:
  libkrbn_configuration_monitor_class(const libkrbn_configuration_monitor_class&) = delete;

  libkrbn_configuration_monitor_class(libkrbn_configuration_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    auto core_configuration_file_path = constants::get_core_configuration_file_path();

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_configuration_directory(), {core_configuration_file_path}},
    };

    file_monitor_ = std::make_unique<file_monitor>(libkrbn::get_logger(),
                                                   targets,
                                                   std::bind(&libkrbn_configuration_monitor_class::cpp_callback, this, std::placeholders::_1));

    cpp_callback(core_configuration_file_path);
  }

private:
  void cpp_callback(const std::string& file_path) {
    if (callback_) {
      core_configuration core_configuration(libkrbn::get_logger(), file_path);
      callback_(core_configuration.get_current_profile_json().c_str(), refcon_);
    }
  }

  libkrbn_log_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<file_monitor> file_monitor_;
};
}

bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor** out, libkrbn_configuration_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_configuration_monitor*>(new libkrbn_configuration_monitor_class(callback, refcon));
  return true;
}

void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor** out) {
  if (out && *out) {
    delete reinterpret_cast<libkrbn_configuration_monitor_class*>(*out);
    *out = nullptr;
  }
}
