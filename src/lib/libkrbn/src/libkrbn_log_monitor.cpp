#include "constants.hpp"
#include "libkrbn.h"
#include <pqrs/spdlog.hpp>

namespace {
class libkrbn_log_lines_class final {
public:
  libkrbn_log_lines_class(std::shared_ptr<std::deque<std::string>> lines) : lines_(lines) {
  }

  std::shared_ptr<std::deque<std::string>> get_lines(void) const {
    return lines_;
  }

private:
  std::shared_ptr<std::deque<std::string>> lines_;
};

class libkrbn_log_monitor_class final {
public:
  libkrbn_log_monitor_class(const libkrbn_log_monitor_class&) = delete;

  libkrbn_log_monitor_class(libkrbn_log_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    std::vector<std::string> targets = {
        "/var/log/karabiner/observer.log",
        "/var/log/karabiner/grabber.log",
    };
    auto log_directory = krbn::constants::get_user_log_directory();
    if (!log_directory.empty()) {
      targets.push_back(log_directory + "/console_user_server.log");
    }

    log_monitor_ = std::make_unique<pqrs::spdlog::monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                           targets,
                                                           250);

    log_monitor_->log_file_updated.connect([this](auto&& lines) {
      if (callback_) {
        auto log_lines = new libkrbn_log_lines_class(lines);
        callback_(reinterpret_cast<libkrbn_log_lines*>(log_lines), refcon_);
        delete log_lines;
      }
    });
  }

  void async_start(void) {
    log_monitor_->async_start(std::chrono::milliseconds(1000));
  }

private:
  libkrbn_log_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<pqrs::spdlog::monitor> log_monitor_;
};
} // namespace

bool libkrbn_log_monitor_initialize(libkrbn_log_monitor** out, libkrbn_log_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_log_monitor*>(new libkrbn_log_monitor_class(callback, refcon));
  return true;
}

void libkrbn_log_monitor_terminate(libkrbn_log_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_log_monitor_class*>(*p);
    *p = nullptr;
  }
}

void libkrbn_log_monitor_start(libkrbn_log_monitor* p) {
  auto log_monitor = reinterpret_cast<libkrbn_log_monitor_class*>(p);
  if (!log_monitor) {
    return;
  }

  log_monitor->async_start();
}

size_t libkrbn_log_lines_get_size(libkrbn_log_lines* p) {
  auto log_lines = reinterpret_cast<libkrbn_log_lines_class*>(p);
  if (!log_lines) {
    return 0;
  }

  auto lines = log_lines->get_lines();
  if (!lines) {
    return 0;
  }

  return lines->size();
}

const char* libkrbn_log_lines_get_line(libkrbn_log_lines* p, size_t index) {
  auto log_lines = reinterpret_cast<libkrbn_log_lines_class*>(p);
  if (!log_lines) {
    return nullptr;
  }

  auto lines = log_lines->get_lines();
  if (!lines) {
    return nullptr;
  }

  if (index >= lines->size()) {
    return nullptr;
  }

  return (*lines)[index].c_str();
}

bool libkrbn_log_lines_is_warn_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::warn;
}

bool libkrbn_log_lines_is_error_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::err;
}
