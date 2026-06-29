#pragma once

// `krbn::console_user_server::console_user_id_changed_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
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
            auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
            if (!result) {
              // During an update, retrieving the Team ID may fail, causing an error once.
              // Since this can occur during normal use, treat it as debug rather than warn.
              logger::get_logger()->debug("console_user_id_changed_client: peer is not code-signed with same Team ID");
            }
            return result;
          });

      client_->connected.connect([this](auto&&) {
        logger::get_logger()->info("console_user_id_changed_client is connected.");

        enqueue_to_dispatcher([this] {
          console_user_id_changed_request_in_flight_ = false;
          connection_ready_ = true;

          connected();
          async_send_pending_console_user_id_changed();
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
        logger::get_logger()->info("console_user_id_changed_client is closed.");

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
        logger::get_logger()->error("console_user_id_changed_client peer_verification_failed");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->received.connect([](auto&&) {
        // Do nothing
      });

      client_->async_start();

      logger::get_logger()->info("console_user_id_changed_client is started.");
    });
  }

  void async_console_user_id_changed(bool on_console) {
    enqueue_to_dispatcher([this, on_console] {
      pending_console_user_id_changed_ = console_user_id_changed_state{
          .on_console = on_console,
      };

      async_send_pending_console_user_id_changed();
    });
  }

private:
  struct console_user_id_changed_state final {
    bool on_console;

    bool operator==(const console_user_id_changed_state&) const = default;
  };

  void stop() {
    if (!client_) {
      return;
    }

    client_ = nullptr;
    pending_console_user_id_changed_ = std::nullopt;
    console_user_id_changed_request_in_flight_ = false;
    connection_ready_ = false;

    logger::get_logger()->info("console_user_id_changed_client is stopped.");
  }

  void make_connection_not_ready() {
    connection_ready_ = false;
    console_user_id_changed_request_in_flight_ = false;
  }

  void async_send_pending_console_user_id_changed() {
    if (!client_ ||
        !connection_ready_ ||
        !pending_console_user_id_changed_ ||
        console_user_id_changed_request_in_flight_) {
      return;
    }

    auto state = *pending_console_user_id_changed_;
    nlohmann::json json{
        {"operation_type", operation_type::console_user_id_changed},
        {"on_console", state.on_console},
    };

    console_user_id_changed_request_in_flight_ = true;
    client_->async_request(
        nlohmann::json::to_msgpack(json),
        [this, state](auto&& error_code, auto&& buffer) {
          enqueue_to_dispatcher([this, state, error_code] {
            console_user_id_changed_request_in_flight_ = false;

            if (error_code) {
              logger::get_logger()->debug("console_user_id_changed_client request failed: {0}", error_code.message());

              make_connection_not_ready();

              return;
            }

            if (pending_console_user_id_changed_ == state) {
              pending_console_user_id_changed_ = std::nullopt;
            } else {
              async_send_pending_console_user_id_changed();
            }
          });
        });
  }

  std::unique_ptr<pqrs::unix_domain_stream::client> client_;
  std::optional<console_user_id_changed_state> pending_console_user_id_changed_;
  bool console_user_id_changed_request_in_flight_ = false;
  bool connection_ready_ = false;
};
} // namespace krbn::console_user_server
