#pragma once

#include "constants.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "settings_callback_manager.hpp"
#include <pqrs/spdlog.hpp>

class settings_log_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_log_monitor(const settings_log_monitor&) = delete;

  settings_log_monitor()
      : dispatcher_client() {
    start();
  }

  ~settings_log_monitor() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void start() {
    if (monitor_) {
      return;
    }

    std::vector<std::string> targets = {
        "/var/log/karabiner/core_service.log",
        "/var/log/karabiner/virtual_hid_device_service.log",
    };
    auto log_directory = krbn::constants::get_user_log_directory();
    if (!log_directory.empty()) {
      targets.push_back(log_directory / "console_user_server.log");
      targets.push_back(log_directory / "core_service.log");
    }

    monitor_ = std::make_unique<pqrs::spdlog::monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                       targets,
                                                       250);

    monitor_->log_file_updated.connect([this](auto&& lines) {
      lines_ = lines;

      auto json_string = make_lines_json_string();
      for (const auto& c : callback_manager_.get_callbacks()) {
        c(json_string.data(), json_string.size());
      }
    });

    monitor_->async_start(std::chrono::milliseconds(1000));
  }

  void stop() {
    monitor_ = nullptr;
  }

  void register_krbn_log_messages_updated_callback(krbn_log_messages_updated_t callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);

      auto json_string = make_lines_json_string();
      callback(json_string.data(), json_string.size());
    });
  }

private:
  [[nodiscard]] std::string make_lines_json_string() const {
    auto json = nlohmann::json::array();

    if (lines_) {
      for (const auto& line : *lines_) {
        if (line.empty()) {
          continue;
        }

        auto level = "info";
        if (auto log_level = pqrs::spdlog::find_level(line)) {
          switch (*log_level) {
            case spdlog::level::debug:
              level = "debug";
              break;
            case spdlog::level::warn:
              level = "warn";
              break;
            case spdlog::level::err:
              level = "error";
              break;
            default:
              break;
          }
        }

        json.push_back({
            {"text", line},
            {"log_level", level},
            {"date_number", pqrs::spdlog::find_date_number(line).value_or(0)},
        });
      }
    }

    return krbn::json_utility::dump(json);
  }

  std::unique_ptr<pqrs::spdlog::monitor> monitor_;
  std::shared_ptr<std::deque<std::string>> lines_;
  settings_callback_manager<krbn_log_messages_updated_t> callback_manager_;
};
