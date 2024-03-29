#pragma once

#include "constants.hpp"
#include "libkrbn/libkrbn.h"
#include "logger.hpp"
#include <pqrs/spdlog.hpp>

class libkrbn_log_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_log_monitor(const libkrbn_log_monitor&) = delete;

  libkrbn_log_monitor(void)
      : dispatcher_client() {
    std::vector<std::string> targets = {
        "/var/log/karabiner/grabber.log",
        "/var/log/karabiner/observer.log",
        "/var/log/karabiner/virtual_hid_device_service.log",
        fmt::format("/var/log/karabiner/session_monitor.{0}.log", geteuid()),
    };
    auto log_directory = krbn::constants::get_user_log_directory();
    if (!log_directory.empty()) {
      targets.push_back(log_directory / "console_user_server.log");
    }

    monitor_ = std::make_unique<pqrs::spdlog::monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                       targets,
                                                       250);

    monitor_->log_file_updated.connect([this](auto&& lines) {
      lines_ = lines;

      for (const auto& c : libkrbn_log_messages_updated_callbacks_) {
        c();
      }
    });

    monitor_->async_start(std::chrono::milliseconds(1000));
  }

  ~libkrbn_log_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  std::shared_ptr<std::deque<std::string>> get_lines(void) const {
    return lines_;
  }

  void register_libkrbn_log_messages_updated_callback(libkrbn_log_messages_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      libkrbn_log_messages_updated_callbacks_.push_back(callback);
    });
  }

  void unregister_libkrbn_log_messages_updated_callback(libkrbn_log_messages_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      libkrbn_log_messages_updated_callbacks_.erase(std::remove_if(std::begin(libkrbn_log_messages_updated_callbacks_),
                                                                   std::end(libkrbn_log_messages_updated_callbacks_),
                                                                   [&](auto& c) {
                                                                     return c == callback;
                                                                   }),
                                                    std::end(libkrbn_log_messages_updated_callbacks_));
    });
  }

private:
  std::unique_ptr<pqrs::spdlog::monitor> monitor_;
  std::shared_ptr<std::deque<std::string>> lines_;
  std::vector<libkrbn_log_messages_updated> libkrbn_log_messages_updated_callbacks_;
};
