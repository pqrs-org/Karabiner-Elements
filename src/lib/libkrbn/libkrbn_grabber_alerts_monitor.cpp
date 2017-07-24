#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "libkrbn.h"

namespace {
class libkrbn_grabber_alerts_monitor_class final {
public:
  libkrbn_grabber_alerts_monitor_class(const libkrbn_grabber_alerts_monitor_class&) = delete;

  libkrbn_grabber_alerts_monitor_class(libkrbn_grabber_alerts_monitor_callback callback,
                                       void* refcon) : callback_(callback),
                                                       refcon_(refcon) {
    auto file_path = krbn::constants::get_grabber_alerts_json_file_path();
    auto directory = krbn::filesystem::dirname(file_path);

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {directory, {file_path}},
    };
    file_monitor_ = std::make_unique<krbn::file_monitor>(targets,
                                                         [this](const std::string&) {
                                                           if (callback_) {
                                                             callback_(refcon_);
                                                           }
                                                         });
  }

private:
  libkrbn_grabber_alerts_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<krbn::file_monitor> file_monitor_;
};
} // namespace

bool libkrbn_grabber_alerts_monitor_initialize(libkrbn_grabber_alerts_monitor** out,
                                               libkrbn_grabber_alerts_monitor_callback callback,
                                               void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_grabber_alerts_monitor*>(new libkrbn_grabber_alerts_monitor_class(callback,
                                                                                                    refcon));
  return true;
}

void libkrbn_grabber_alerts_monitor_terminate(libkrbn_grabber_alerts_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_grabber_alerts_monitor_class*>(*p);
    *p = nullptr;
  }
}
