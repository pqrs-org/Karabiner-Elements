#include "constants.hpp"
#include "libkrbn.h"
#include <pqrs/filesystem.hpp>
#include <pqrs/osx/file_monitor.hpp>

namespace {
class libkrbn_file_monitor_class final {
public:
  libkrbn_file_monitor_class(const libkrbn_file_monitor_class&) = delete;

  libkrbn_file_monitor_class(const char* file_path,
                             libkrbn_file_monitor_callback callback,
                             void* refcon) {
    if (file_path) {
      std::vector<std::string> targets = {
          file_path,
      };
      file_monitor_ = std::make_unique<pqrs::osx::file_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                targets);

      file_monitor_->file_changed.connect([callback, refcon](auto&& changed_file_path,
                                                             auto&& changed_file_body) {
        if (callback) {
          callback(refcon);
        }
      });

      file_monitor_->async_start();
    }
  }

  ~libkrbn_file_monitor_class(void) {
    file_monitor_ = nullptr;
  }

private:
  std::unique_ptr<pqrs::osx::file_monitor> file_monitor_;
};
} // namespace

bool libkrbn_file_monitor_initialize(libkrbn_file_monitor** out,
                                     const char* file_path,
                                     libkrbn_file_monitor_callback callback,
                                     void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_file_monitor*>(new libkrbn_file_monitor_class(file_path,
                                                                                callback,
                                                                                refcon));
  return true;
}

void libkrbn_file_monitor_terminate(libkrbn_file_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_file_monitor_class*>(*p);
    *p = nullptr;
  }
}
