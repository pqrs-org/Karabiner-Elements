#pragma once

// `krbn::console_user_server_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <unistd.h>
#include <utility>
#include <vector>

namespace krbn {
class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  explicit console_user_server_client(uid_t uid)
      : dispatcher_client(),
        uid_(uid) {
  }

  ~console_user_server_client() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_server_client is already started.");
        return;
      }

      client_ = std::make_unique<pqrs::unix_domain_stream::client>(
          weak_dispatcher_,
          constants::get_console_user_server_socket_file_path(uid_),
          constants::get_unix_domain_stream_client_options(),
          [](const auto& peer_credentials) {
            return get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          });

      client_->connected.connect([this](auto&&) {
        logger::get_logger()->debug("console_user_server_client is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->debug("console_user_server_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->closed.connect([this] {
        logger::get_logger()->debug("console_user_server_client is closed.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->debug("console_user_server_client error: {0}", error_code.message());
      });

      client_->peer_verification_failed.connect([](auto&&) {
        // During an update, retrieving the Team ID may fail, causing an error once.
        // Since this can occur during normal use, treat it as debug rather than warn.
        logger::get_logger()->debug("console_user_server_client peer_verification_failed");
      });

      client_->received.connect([this](auto&& buffer) {
        enqueue_to_dispatcher([this, buffer] {
          if (buffer->empty()) {
            return;
          }

          try {
            auto json = nlohmann::json::from_msgpack(*buffer);
            auto ot = json.at("operation_type").template get<operation_type>();

            received(ot,
                     json);
          } catch (std::exception& e) {
            logger::get_logger()->error("received data is corrupted");
          }
        });
      });

      client_->request_received.connect([](auto, auto&&) {
        // Do nothing
      });

      client_->async_start();

      logger::get_logger()->debug("console_user_server_client is started.");
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  // settings_window -> console_user_server
  void async_get_settings_window_guidance() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_settings_window_guidance},
      };

      async_request(std::move(json));
    });
  }

  // event_viewer -> console_user_server
  void async_get_frontmost_application_history() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_frontmost_application_history},
      };

      async_request(std::move(json));
    });
  }

private:
  void async_request(nlohmann::json&& json) const {
    if (!client_) {
      return;
    }

    client_->async_request(
        nlohmann::json::to_msgpack(json),
        [this](auto&& error_code, auto&& buffer) {
          enqueue_to_dispatcher([this, error_code, buffer] {
            if (error_code) {
              logger::get_logger()->debug("console_user_server_client request failed: {0}", error_code.message());
            }

            if (buffer &&
                !buffer->empty()) {
              handle_message(buffer);
            }
          });
        });
  }

  void handle_message(pqrs::not_null_shared_ptr_t<std::vector<uint8_t>> buffer) const {
    try {
      auto json = nlohmann::json::from_msgpack(*buffer);
      auto ot = json.at("operation_type").template get<operation_type>();
      if (ot == operation_type::none) {
        return;
      }

      received(ot,
               json);
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
    }
  }

  void stop() {
    if (!client_) {
      return;
    }

    client_ = nullptr;

    logger::get_logger()->debug("console_user_server_client is stopped.");
  }

  uid_t uid_;

  std::unique_ptr<pqrs::unix_domain_stream::client> client_;
};
} // namespace krbn
