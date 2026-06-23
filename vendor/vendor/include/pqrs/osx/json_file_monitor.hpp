#pragma once

// pqrs::osx::json_file_monitor v1.2.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
#include <pqrs/osx/file_monitor.hpp>

namespace pqrs::osx {
class json_file_monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const std::string& changed_file_path, std::shared_ptr<nlohmann::json> json)> json_file_changed;
  nod::signal<void(const std::string&)> error_occurred;
  nod::signal<void(const std::string& file_path, const std::string& message)> json_error_occurred;

  // Methods

  json_file_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                    const std::vector<std::string>& files) : dispatcher_client(weak_dispatcher) {
    file_monitor_ = std::make_unique<file_monitor>(weak_dispatcher,
                                                   files);

    file_monitor_->file_changed.connect([this](auto&& changed_file_path, auto&& changed_file_body) {
      std::shared_ptr<nlohmann::json> json;

      if (changed_file_body) {
        try {
          json = std::make_shared<nlohmann::json>(
              nlohmann::json::parse(std::begin(*changed_file_body),
                                    std::end(*changed_file_body)));
        } catch (const std::exception& e) {
          std::string message(e.what());
          json_error_occurred(changed_file_path, message);
          return;
        }
      }

      json_file_changed(changed_file_path, json);
    });

    file_monitor_->error_occurred.connect([this](auto&& message) {
      error_occurred(message);
    });
  }

  ~json_file_monitor() override {
    // dispatcher_client

    detach_from_dispatcher([this] {
      file_monitor_ = nullptr;
    });
  }

  void async_start() {
    file_monitor_->async_start();
  }

private:
  std::unique_ptr<file_monitor> file_monitor_;
};
} // namespace pqrs::osx
