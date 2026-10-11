#pragma once

// `krbn::console_user_server_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/xpc.hpp>
#include <unistd.h>
#include <utility>
#include <vector>

namespace krbn {
namespace xpc = pqrs::osx::xpc;

class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const std::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client() {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~console_user_server_client() override {
    unregister_callbacks_and_detach();
  }

  // Stop callbacks and dispatcher activity at the owner's lifecycle boundary.
  //
  // Do not rely only on the destructor for this cleanup. A caller can hold a
  // temporary shared_ptr after the owner releases its reference, which would
  // otherwise leave callbacks that capture the owner alive.
  void unregister_callbacks_and_detach() {
    std::call_once(unregister_callbacks_and_detach_once_, [this] {
      detach_from_dispatcher([this] {
        connected.disconnect_all_slots();
        connect_failed.disconnect_all_slots();
        closed.disconnect_all_slots();
        received.disconnect_all_slots();

        stop();
      });
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_server_client is already started.");
        return;
      }

      client_ = std::make_unique<xpc::client>(
          weak_dispatcher_,
          xpc::client_options({
              .common_parameters = {
                  .signing_requirement = get_shared_codesign_manager()->xpc_signing_requirement({
                      "org.pqrs.Karabiner-Console-User-Server",
                  }),
                  .max_message_size = constants::xpc_max_message_size,
              },
              .client_parameters = {
                  .service_name = constants::get_console_user_server_mach_service_name(),
              },
          }));

      client_->peer_ready.connect([this](auto peer_id) {
        peer_id_ = peer_id;
        logger::get_logger()->debug("console_user_server_client is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connection_failed.connect([this](const auto& error_code) {
        logger::get_logger()->debug("console_user_server_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->peer_interrupted.connect([this](auto peer_id) {
        if (peer_id_ == peer_id) {
          peer_id_.reset();
        }
        logger::get_logger()->debug("console_user_server_client is interrupted.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->peer_invalidated.connect([this](auto peer_id,
                                               const auto& error_code) {
        if (peer_id_ == peer_id) {
          peer_id_.reset();
        }
        logger::get_logger()->debug("console_user_server_client peer_invalidated: {0}",
                                    error_code.message());

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](const auto& error) {
        logger::get_logger()->debug("console_user_server_client error: {0}",
                                    error.message);
      });

      client_->message_received.connect([this](auto, auto&& buffer) {
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

      client_->request_received.connect([this](auto,
                                               auto reply,
                                               auto&& buffer) {
        if (client_) {
          client_->async_reply(
              reply,
              std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(nlohmann::json{
                  {"operation_type", operation_type::none},
              })));
        }

        if (!buffer->empty()) {
          handle_message(buffer);
        }
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
    if (!client_ ||
        !peer_id_) {
      return;
    }

    // The listener checks this UID against its console account.
    json["uid"] = getuid();

    client_->async_request(
        xpc::client::async_request_parameters{
            .id = *peer_id_,
            .data = std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)),
            .completion = [this](auto result) {
              enqueue_to_dispatcher([this, result] {
                if (!result) {
                  logger::get_logger()->debug("console_user_server_client request failed: {0}",
                                              result.error().message());
                  return;
                }

                if (!(*result)->empty()) {
                  handle_message(*result);
                }
              });
            },
        });
  }

  void handle_message(pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>> buffer) const {
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
    peer_id_.reset();

    logger::get_logger()->debug("console_user_server_client is stopped.");
  }

  std::unique_ptr<xpc::client> client_;
  std::optional<xpc::peer_id> peer_id_;
  std::once_flag unregister_callbacks_and_detach_once_;
};
} // namespace krbn
