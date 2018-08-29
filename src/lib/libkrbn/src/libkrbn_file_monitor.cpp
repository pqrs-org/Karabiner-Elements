#include "constants.hpp"
#include "filesystem.hpp"
#include "libkrbn.h"
#include "monitor/file_monitor.hpp"

namespace {
class libkrbn_file_monitor_class final {
public:
  libkrbn_file_monitor_class(const libkrbn_file_monitor_class&) = delete;

  libkrbn_file_monitor_class(const char* file_path,
                             libkrbn_file_monitor_callback callback,
                             void* refcon) : callback_(callback),
                                             refcon_(refcon) {
    if (file_path) {
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
  }

private:
  libkrbn_file_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<krbn::file_monitor> file_monitor_;
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
