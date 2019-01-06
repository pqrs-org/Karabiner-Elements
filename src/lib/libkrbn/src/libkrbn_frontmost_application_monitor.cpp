#include "constants.hpp"
#include "libkrbn.h"
#include <pqrs/osx/frontmost_application_monitor.hpp>

namespace {
class libkrbn_frontmost_application_monitor_class final {
public:
  libkrbn_frontmost_application_monitor_class(const libkrbn_frontmost_application_monitor_class&) = delete;

  libkrbn_frontmost_application_monitor_class(libkrbn_frontmost_application_monitor_callback callback,
                                              void* refcon) {
    monitor_ = std::make_unique<pqrs::osx::frontmost_application_monitor::monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    monitor_->frontmost_application_changed.connect([callback, refcon](auto&& application_ptr) {
      if (application_ptr && callback) {
        std::string bundle_identifier;
        if (auto& bi = application_ptr->get_bundle_identifier()) {
          bundle_identifier = *bi;
        }

        std::string file_path;
        if (auto& fp = application_ptr->get_file_path()) {
          file_path = *fp;
        }

        callback(bundle_identifier.c_str(),
                 file_path.c_str(),
                 refcon);
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_frontmost_application_monitor_class(void) {
    monitor_ = nullptr;
  }

private:
  std::unique_ptr<pqrs::osx::frontmost_application_monitor::monitor> monitor_;
};
} // namespace

bool libkrbn_frontmost_application_monitor_initialize(libkrbn_frontmost_application_monitor** out, libkrbn_frontmost_application_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_frontmost_application_monitor*>(new libkrbn_frontmost_application_monitor_class(callback, refcon));
  return true;
}

void libkrbn_frontmost_application_monitor_terminate(libkrbn_frontmost_application_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_frontmost_application_monitor_class*>(*p);
    *p = nullptr;
  }
}
