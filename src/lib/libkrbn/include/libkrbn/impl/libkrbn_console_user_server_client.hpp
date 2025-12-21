#pragma once

#include "console_user_server_client_v2.hpp"
#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"

class libkrbn_console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_console_user_server_client(const libkrbn_console_user_server_client&) = delete;

  libkrbn_console_user_server_client(uid_t uid,
                                     std::optional<std::string> client_socket_directory_name)
      : dispatcher_client(),
        status_(libkrbn_console_user_server_client_status_none) {
    console_user_server_client_ = std::make_unique<krbn::console_user_server_client_v2>(uid,
                                                                                        client_socket_directory_name);

    console_user_server_client_->connected.connect([this] {
      set_status(libkrbn_console_user_server_client_status_connected);
    });

    console_user_server_client_->connect_failed.connect([this](auto&& error_code) {
      set_status(libkrbn_console_user_server_client_status_connect_failed);
    });

    console_user_server_client_->closed.connect([this] {
      set_status(libkrbn_console_user_server_client_status_closed);
    });

    console_user_server_client_->received.connect([this](auto&& operation_type,
                                                         auto&& json) {
      try {
        switch (json.at("operation_type").template get<krbn::operation_type>()) {
          case krbn::operation_type::frontmost_application_history: {
            auto json_dump = krbn::json_utility::dump(json.at("frontmost_application_history"));

            for (const auto& c : frontmost_application_history_received_callback_manager_.get_callbacks()) {
              c(json_dump.c_str());
            }

            break;
          }

          default:
            break;
        }
      } catch (std::exception& e) {
        krbn::logger::get_logger()->error("libkrbn_console_user_server_client received data is corrupted");
      }
    });
  }

  ~libkrbn_console_user_server_client(void) {
    detach_from_dispatcher([this] {
      console_user_server_client_ = nullptr;
    });
  }

  void async_start(void) const {
    console_user_server_client_->async_start();
  }

  libkrbn_console_user_server_client_status get_status(void) const {
    return status_;
  }

  void async_get_frontmost_application_history(void) {
    console_user_server_client_->async_get_frontmost_application_history();
  }

  void register_libkrbn_console_user_server_client_status_changed_callback(libkrbn_console_user_server_client_status_changed_t callback) {
    enqueue_to_dispatcher([this, callback] {
      status_changed_callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_console_user_server_client_status_changed_callback(libkrbn_console_user_server_client_status_changed_t callback) {
    enqueue_to_dispatcher([this, callback] {
      status_changed_callback_manager_.unregister_callback(callback);
    });
  }

  void register_libkrbn_console_user_server_client_frontmost_application_history_received_callback(libkrbn_console_user_server_client_frontmost_application_history_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      frontmost_application_history_received_callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_console_user_server_client_frontmost_application_history_received_callback(libkrbn_console_user_server_client_frontmost_application_history_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      frontmost_application_history_received_callback_manager_.unregister_callback(callback);
    });
  }

private:
  // This method should be called in the shared dispatcher thread.
  void set_status(libkrbn_console_user_server_client_status status) {
    status_ = status;

    for (const auto& c : status_changed_callback_manager_.get_callbacks()) {
      c();
    }
  }

  std::unique_ptr<krbn::console_user_server_client_v2> console_user_server_client_;
  libkrbn_console_user_server_client_status status_;
  libkrbn_callback_manager<libkrbn_console_user_server_client_status_changed_t> status_changed_callback_manager_;
  libkrbn_callback_manager<libkrbn_console_user_server_client_frontmost_application_history_received_t> frontmost_application_history_received_callback_manager_;
};
