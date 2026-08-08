#pragma once

#include "console_user_server_client.hpp"
#include "settings.hpp"
#include <atomic>
#include <memory>

class settings_console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_console_user_server_client(const settings_console_user_server_client&) = delete;

  settings_console_user_server_client(uid_t uid,
                                      krbn_console_user_server_client_status_changed_t status_changed_callback,
                                      krbn_console_user_server_client_settings_window_guidance_received_t settings_window_guidance_received_callback)
      : dispatcher_client(),
        uid_(uid),
        connected_(false),
        status_changed_callback_(status_changed_callback),
        settings_window_guidance_received_callback_(settings_window_guidance_received_callback) {
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
      set_connected(true);
    });

    client->connect_failed.connect([this](auto&&) {
      set_connected(false);
    });

    client->closed.connect([this] {
      set_connected(false);
    });

    client->received.connect([this](auto&& operation_type,
                                    auto&& json) {
      if (operation_type != krbn::operation_type::settings_window_guidance) {
        return;
      }

      try {
        auto json_dump = krbn::json_utility::dump(json.at("settings_window_guidance"));
        settings_window_guidance_received_callback_(json_dump.c_str());
      } catch (const std::exception&) {
        krbn::logger::get_logger()->error("settings_console_user_server_client received data is corrupted");
      }
    });

    std::atomic_store(&console_user_server_client_, client);
  }

  void stop() {
    std::atomic_store(&console_user_server_client_,
                      std::shared_ptr<krbn::console_user_server_client>());
    connected_.store(false);
  }

  void async_start() const {
    if (auto client = std::atomic_load(&console_user_server_client_)) {
      client->async_start();
    }
  }

  [[nodiscard]] bool connected() const {
    return connected_.load();
  }

  void async_get_settings_window_guidance() const {
    if (auto client = std::atomic_load(&console_user_server_client_)) {
      client->async_get_settings_window_guidance();
    }
  }

private:
  // This method should be called in the shared dispatcher thread.
  void set_connected(bool value) {
    if (connected_.exchange(value) != value) {
      status_changed_callback_();
    }
  }

  uid_t uid_;
  std::shared_ptr<krbn::console_user_server_client> console_user_server_client_;
  std::atomic_bool connected_;
  const krbn_console_user_server_client_status_changed_t status_changed_callback_;
  const krbn_console_user_server_client_settings_window_guidance_received_t settings_window_guidance_received_callback_;
};
