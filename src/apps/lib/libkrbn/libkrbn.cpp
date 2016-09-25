#include "libkrbn.h"
#include "constants.hpp"
#include "log_monitor.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

class libkrbn {
public:
  static bool cfstring_to_cstring(std::vector<char>& v, CFStringRef string) {
    if (string) {
      if (auto cstring = CFStringGetCStringPtr(string, kCFStringEncodingUTF8)) {
        auto length = strlen(cstring) + 1;
        v.resize(length);
        strlcpy(&(v[0]), cstring, length);
        return true;
      }
    }

    v.resize(1);
    v[0] = '\0';
    return false;
  }

  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    static std::shared_ptr<spdlog::logger> logger;

    std::lock_guard<std::mutex> guard(mutex);

    if (!logger) {
      logger = spdlog::stdout_logger_mt("libkrbn", true);
    }
    return *logger;
  }
};

const char* libkrbn_get_distributed_notification_observed_object(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_observed_object());
  }

  return &(result[0]);
}

const char* libkrbn_get_distributed_notification_grabber_is_launched(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_grabber_is_launched());
  }

  return &(result[0]);
}

class libkrbn_log_monitor_class {
public:
  libkrbn_log_monitor_class(const libkrbn_log_monitor_class&) = delete;

  libkrbn_log_monitor_class(libkrbn_log_monitor_callback callback) : callback_(callback) {
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

private:
  void cpp_callback(const std::string& line) {
    if (callback_) {
      callback_(line.c_str());
    }
  }

  libkrbn_log_monitor_callback callback_;

  std::unique_ptr<log_monitor> log_monitor_;
};

bool libkrbn_log_monitor_initialize(libkrbn_log_monitor** out, libkrbn_log_monitor_callback callback) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_log_monitor*>(new libkrbn_log_monitor_class(callback));
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
