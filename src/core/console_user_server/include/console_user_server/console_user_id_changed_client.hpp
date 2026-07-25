#pragma once

// `krbn::console_user_server::console_user_id_changed_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "console_user_id_changed_client_state.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <pqrs/unix_domain_stream.hpp>

namespace krbn::console_user_server {
class console_user_id_changed_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(std::optional<uid_t>)> core_service_daemon_server_bound;

  // Methods

  console_user_id_changed_client(const console_user_id_changed_client&) = delete;

  console_user_id_changed_client()
      : dispatcher_client() {
  }

  ~console_user_id_changed_client() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_id_changed_client is already started.");
        return;
      }

      client_ = std::make_unique<pqrs::unix_domain_stream::client>(
          weak_dispatcher_,
          constants::get_console_user_id_changed_receiver_socket_file_path(),
          constants::get_unix_domain_stream_client_options(),
          [](const auto& peer_credentials) {
            return get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          });

      client_->connected.connect([this](auto&&) {
        logger::get_logger()->debug("console_user_id_changed_client is connected.");

        enqueue_to_dispatcher([this] {
          state_.connected();

          connected();
          async_deliver_pending_console_user_id_changed();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->debug("console_user_id_changed_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          make_connection_not_ready();

          connect_failed(error_code);
        });
      });

      client_->closed.connect([this] {
        logger::get_logger()->debug("console_user_id_changed_client is closed.");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();

          closed();
        });
      });

      client_->error_occurred.connect([this](auto&& error_code) {
        logger::get_logger()->debug("console_user_id_changed_client error: {0}", error_code.message());

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->peer_verification_failed.connect([this](auto&&) {
        // During an update, retrieving the Team ID may fail, causing an error once.
        // Since this can occur during normal use, treat it as debug rather than warn.
        logger::get_logger()->debug("console_user_id_changed_client peer_verification_failed");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->received.connect([this](auto&& buffer) {
        enqueue_to_dispatcher([this, buffer] {
          if (buffer->empty()) {
            return;
          }

          try {
            auto json = nlohmann::json::from_msgpack(*buffer);
            switch (json.at("operation_type").template get<operation_type>()) {
              case operation_type::core_service_daemon_server_bound: {
                std::optional<uid_t> uid;
                if (!json.at("uid").is_null()) {
                  uid = json.at("uid").template get<uid_t>();
                }

                state_.core_service_daemon_server_bound(uid);
                notify_core_service_daemon_server_bound_if_ready();
                break;
              }

              default:
                break;
            }
          } catch (std::exception& e) {
            logger::get_logger()->error("console_user_id_changed_client received data is corrupted");
          }
        });
      });

      client_->request_received.connect([](auto, auto&&) {
        // Do nothing
      });

      client_->async_start();

      logger::get_logger()->debug("console_user_id_changed_client is started.");
    });
  }

  void async_console_user_id_changed(bool on_console) {
    enqueue_to_dispatcher([this, on_console] {
      state_.console_user_id_changed(on_console);

      async_deliver_pending_console_user_id_changed();
    });
  }

private:
  void stop() {
    if (!client_) {
      return;
    }

    client_ = nullptr;
    state_.stop();

    logger::get_logger()->debug("console_user_id_changed_client is stopped.");
  }

  void make_connection_not_ready() {
    state_.connection_not_ready();
  }

  void notify_core_service_daemon_server_bound_if_ready() {
    if (auto action = state_.take_core_service_daemon_server_bound_action()) {
      core_service_daemon_server_bound(action->uid);
    }
  }

  void async_deliver_pending_console_user_id_changed() {
    if (!client_) {
      return;
    }

    auto request = state_.take_console_user_id_changed_request();
    if (!request) {
      return;
    }

    nlohmann::json json{
        {"operation_type", operation_type::console_user_id_changed},
        {"on_console", request->on_console},
    };

    client_->async_request(
        nlohmann::json::to_msgpack(json),
        [this, request = *request](auto&& error_code, auto&& buffer) {
          enqueue_to_dispatcher([this, request, error_code] {
            if (error_code) {
              logger::get_logger()->debug("console_user_id_changed_client request failed: {0}", error_code.message());

              make_connection_not_ready();

              return;
            }

            state_.console_user_id_changed_request_succeeded(request);
            notify_core_service_daemon_server_bound_if_ready();
            async_deliver_pending_console_user_id_changed();
          });
        });
  }

  std::unique_ptr<pqrs::unix_domain_stream::client> client_;
  console_user_id_changed_client_state state_;
};
} // namespace krbn::console_user_server
