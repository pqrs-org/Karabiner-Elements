#pragma once

#include "console_user_server_client.hpp"
#include "settings.hpp"
#include "settings_callback_manager.hpp"

class settings_console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_console_user_server_client(const settings_console_user_server_client&) = delete;

  explicit settings_console_user_server_client(uid_t uid)
      : dispatcher_client(),
        console_user_server_client_(std::make_unique<krbn::console_user_server_client>(uid)),
        status_(krbn_console_user_server_client_status_none) {
    console_user_server_client_->connected.connect([this] {
      set_status(krbn_console_user_server_client_status_connected);
    });

    console_user_server_client_->connect_failed.connect([this](auto&&) {
      set_status(krbn_console_user_server_client_status_connect_failed);
    });

    console_user_server_client_->closed.connect([this] {
      set_status(krbn_console_user_server_client_status_closed);
    });

    console_user_server_client_->received.connect([this](auto&& operation_type,
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
  }

  ~settings_console_user_server_client() override {
    detach_from_dispatcher([this] {
      console_user_server_client_ = nullptr;
    });
  }

  void async_start() const {
    console_user_server_client_->async_start();
  }

  [[nodiscard]] krbn_console_user_server_client_status get_status() const {
    return status_;
  }

  void async_get_settings_window_guidance() const {
    console_user_server_client_->async_get_settings_window_guidance();
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
    status_ = status;

    for (const auto& callback : status_changed_callback_manager_.get_callbacks()) {
      callback();
    }
  }

  std::unique_ptr<krbn::console_user_server_client> console_user_server_client_;
  krbn_console_user_server_client_status status_;
  settings_callback_manager<krbn_console_user_server_client_status_changed_t> status_changed_callback_manager_;
  settings_callback_manager<krbn_console_user_server_client_settings_window_guidance_received_t> settings_window_guidance_received_callback_manager_;
};
