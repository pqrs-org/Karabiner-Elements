#pragma once

#include "console_user_server_client.hpp"
#include "settings.hpp"
#include "settings_callback_manager.hpp"
#include <atomic>
#include <memory>

class settings_console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_console_user_server_client(const settings_console_user_server_client&) = delete;

  explicit settings_console_user_server_client(uid_t uid)
      : dispatcher_client(),
        uid_(uid),
        status_(krbn_console_user_server_client_status_none) {
    start();
  }

  ~settings_console_user_server_client() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void start() {
    if (std::atomic_load(&console_user_server_client_)) {
      return;
    }

    auto client = std::make_shared<krbn::console_user_server_client>(uid_);

    client->connected.connect([this] {
      set_status(krbn_console_user_server_client_status_connected);
    });

    client->connect_failed.connect([this](auto&&) {
      set_status(krbn_console_user_server_client_status_connect_failed);
    });

    client->closed.connect([this] {
      set_status(krbn_console_user_server_client_status_closed);
    });

    client->received.connect([this](auto&& operation_type,
                                    auto&& json) {
      if (operation_type != krbn::operation_type::settings_window_guidance) {
        return;
      }

      try {
        auto json_dump = krbn::json_utility::dump(json.at("settings_window_guidance"));
        for (const auto& callback : settings_window_guidance_received_callback_manager_.get_callbacks()) {
          callback(json_dump.c_str());
        }
      } catch (const std::exception&) {
        krbn::logger::get_logger()->error("settings_console_user_server_client received data is corrupted");
      }
    });

    std::atomic_store(&console_user_server_client_, client);
  }

  void stop() {
    std::atomic_store(&console_user_server_client_,
                      std::shared_ptr<krbn::console_user_server_client>());
    status_.store(krbn_console_user_server_client_status_none);
  }

  void async_start() const {
    if (auto client = std::atomic_load(&console_user_server_client_)) {
      client->async_start();
    }
  }

  [[nodiscard]] krbn_console_user_server_client_status get_status() const {
    return status_.load();
  }

  void async_get_settings_window_guidance() const {
    if (auto client = std::atomic_load(&console_user_server_client_)) {
      client->async_get_settings_window_guidance();
    }
  }

  void register_status_changed_callback(krbn_console_user_server_client_status_changed_t callback) {
    enqueue_to_dispatcher([this, callback] {
      status_changed_callback_manager_.register_callback(callback);
    });
  }

  void register_settings_window_guidance_received_callback(krbn_console_user_server_client_settings_window_guidance_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      settings_window_guidance_received_callback_manager_.register_callback(callback);
    });
  }

private:
  // This method should be called in the shared dispatcher thread.
  void set_status(krbn_console_user_server_client_status status) {
    status_.store(status);

    for (const auto& callback : status_changed_callback_manager_.get_callbacks()) {
      callback();
    }
  }

  uid_t uid_;
  std::shared_ptr<krbn::console_user_server_client> console_user_server_client_;
  std::atomic<krbn_console_user_server_client_status> status_;
  settings_callback_manager<krbn_console_user_server_client_status_changed_t> status_changed_callback_manager_;
  settings_callback_manager<krbn_console_user_server_client_settings_window_guidance_received_t> settings_window_guidance_received_callback_manager_;
};
