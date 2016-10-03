#include "constants.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"
#include "log_monitor.hpp"

namespace {
class libkrbn_log_monitor_class {
public:
  libkrbn_log_monitor_class(const libkrbn_log_monitor_class&) = delete;

  libkrbn_log_monitor_class(libkrbn_log_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    std::vector<std::string> targets = {
        "/var/log/karabiner/grabber_log",
        "/var/log/karabiner/event_dispatcher_log",
    };
    if (auto p = constants::get_home_dot_karabiner_directory()) {
      targets.push_back(std::string(p) + "/log/console_user_server_log");
    }

    log_monitor_ = std::make_unique<log_monitor>(libkrbn::get_logger(),
                                                 targets,
                                                 std::bind(&libkrbn_log_monitor_class::cpp_callback, this, std::placeholders::_1));
  }

  const std::vector<std::pair<uint64_t, std::string>>& get_initial_lines(void) const {
    return log_monitor_->get_initial_lines();
  }

  void start(void) {
    log_monitor_->start();
  }

private:
  void cpp_callback(const std::string& line) {
    if (callback_) {
      callback_(line.c_str(), refcon_);
    }
  }

  libkrbn_log_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<log_monitor> log_monitor_;
};
}

bool libkrbn_log_monitor_initialize(libkrbn_log_monitor** out, libkrbn_log_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_log_monitor*>(new libkrbn_log_monitor_class(callback, refcon));
  return true;
}

void libkrbn_log_monitor_terminate(libkrbn_log_monitor** out) {
  if (out && *out) {
    delete reinterpret_cast<libkrbn_log_monitor_class*>(*out);
    *out = nullptr;
  }
}

size_t libkrbn_log_monitor_initial_lines_size(libkrbn_log_monitor* p) {
  auto log_monitor = reinterpret_cast<libkrbn_log_monitor_class*>(p);
  if (!log_monitor) {
    return 0;
  }

  auto& initial_lines = log_monitor->get_initial_lines();
  return initial_lines.size();
}

const char* libkrbn_log_monitor_initial_line(libkrbn_log_monitor* p, size_t index) {
  auto log_monitor = reinterpret_cast<libkrbn_log_monitor_class*>(p);
  if (!log_monitor) {
    return 0;
  }

  auto& initial_lines = log_monitor->get_initial_lines();
  if (index >= initial_lines.size()) {
    return nullptr;
  }

  return initial_lines[index].second.c_str();
}

void libkrbn_log_monitor_start(libkrbn_log_monitor* p) {
  auto log_monitor = reinterpret_cast<libkrbn_log_monitor_class*>(p);
  if (!log_monitor) {
    return;
  }

  log_monitor->start();
}
