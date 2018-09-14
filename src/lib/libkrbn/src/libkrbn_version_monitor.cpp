#include "constants.hpp"
#include "libkrbn.h"
#include "monitor/version_monitor.hpp"

namespace {
class libkrbn_version_monitor_class final {
public:
  libkrbn_version_monitor_class(const libkrbn_version_monitor_class&) = delete;

  libkrbn_version_monitor_class(libkrbn_version_monitor_callback callback,
                                void* refcon) {
    version_monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    version_monitor_->changed.connect([callback, refcon](auto&& version) {
      if (callback) {
        callback(refcon);
      }
    });

    version_monitor_->async_start();
  }

  ~libkrbn_version_monitor_class(void) {
    version_monitor_ = nullptr;
  }

private:
  std::unique_ptr<krbn::version_monitor> version_monitor_;
};
} // namespace

bool libkrbn_version_monitor_initialize(libkrbn_version_monitor** out, libkrbn_version_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_version_monitor*>(new libkrbn_version_monitor_class(callback, refcon));
  return true;
}

void libkrbn_version_monitor_terminate(libkrbn_version_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_version_monitor_class*>(*p);
    *p = nullptr;
  }
}
