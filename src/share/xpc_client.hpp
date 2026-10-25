#pragma once

// `krbn::xpc_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/xpc.hpp>
#include <string>
#include <utility>
#include <vector>

namespace krbn {
namespace xpc = pqrs::osx::xpc;

class xpc_client : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  struct parameters final {
    std::string client_name;
    std::string service_name;
    std::string signing_identifier;
    bool privileged{false};
  };

  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const std::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  xpc_client(const xpc_client&) = delete;

  explicit xpc_client(parameters parameters)
      : parameters_(std::move(parameters)) {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~xpc_client() override {
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
        logger::get_logger()->warn("{0} is already started.",
                                   parameters_.client_name);
        return;
      }

      running_ = true;
      auto client_generation = ++client_generation_;

      client_ = std::make_unique<xpc::client>(
          weak_dispatcher_,
          xpc::client_options({
              .common_parameters = {
                  .signing_requirement = get_shared_codesign_manager()->xpc_signing_requirement({
                      parameters_.signing_identifier,
                  }),
                  .max_message_size = constants::xpc_max_message_size,
              },
              .client_parameters = {
                  .service_name = parameters_.service_name,
                  .privileged = parameters_.privileged,
              },
          }));

      client_->peer_ready.connect([this](const auto& context) {
        peer_id_ = context.id;
        logger::get_logger()->debug("{0} is connected.",
                                    parameters_.client_name);

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connection_retry_scheduled.connect([this](const auto& error_code) {
        logger::get_logger()->debug("{0} connect_failed: {1}",
                                    parameters_.client_name,
                                    error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->peer_interrupted.connect([this](const auto& context) {
        logger::get_logger()->debug("{0} is interrupted.",
                                    parameters_.client_name);

        handle_peer_closed(context.id);
      });

      client_->peer_invalidated.connect([this](const auto& context,
                                               const auto& error_code) {
        logger::get_logger()->debug("{0} peer_invalidated: {1}",
                                    parameters_.client_name,
                                    error_code.message());

        handle_peer_closed(context.id);
      });

      client_->client_recovery_required.connect([this, client_generation](const auto& request) {
        logger::get_logger()->debug("{0} recovery required: {1}",
                                    parameters_.client_name,
                                    request.error.message());

        handle_client_recovery_request(request,
                                       client_generation);
      });

      client_->message_received.connect([this](const auto&, auto&& buffer) {
        enqueue_to_dispatcher([this, buffer] {
          if (!buffer->empty()) {
            handle_message(buffer);
          }
        });
      });

      client_->request_received.connect([this](const auto& context,
                                               auto&& buffer) {
        // Reply before handling the message, since some handlers run synchronous helper commands
        // such as service registration, which may exceed the request timeout and close the peer.
        // This response means the request has been accepted, not fully processed.
        if (client_) {
          client_->async_reply(
              context.reply,
              std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(nlohmann::json{
                  {"operation_type", operation_type::none},
              })));
        }

        if (!buffer->empty()) {
          handle_message(buffer);
        }
      });

      client_->async_start();

      logger::get_logger()->debug("{0} is started.",
                                  parameters_.client_name);
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

protected:
  using request_completion_handler = std::function<void(const std::error_code&)>;

  void async_request(nlohmann::json&& json,
                     request_completion_handler completion_handler = nullptr) const {
    if (!client_ ||
        !peer_id_) {
      if (completion_handler) {
        completion_handler(xpc::make_error_code(xpc::errc::not_ready));
      }
      return;
    }

    client_->async_request(
        xpc::client::async_request_parameters{
            .id = *peer_id_,
            .data = std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)),
            .completion = [this, completion_handler = std::move(completion_handler)](auto result) {
              enqueue_to_dispatcher([this, result, completion_handler = std::move(completion_handler)] {
                if (!result) {
                  logger::get_logger()->debug("{0} request failed: {1}",
                                              parameters_.client_name,
                                              result.error().message());
                }

                if (result &&
                    !(*result)->empty()) {
                  handle_message(*result);
                }

                if (completion_handler) {
                  completion_handler(result
                                         ? std::error_code{}
                                         : result.error());
                }
              });
            },
        });
  }

private:
  void handle_client_recovery_request(const xpc::client_recovery_request& request,
                                      std::uint64_t client_generation) {
    if (peer_id_) {
      handle_peer_closed(*peer_id_);
    } else {
      enqueue_to_dispatcher([this, error = request.error] {
        connect_failed(error);
      });
    }

    // client_recovery_required is terminal for this client instance. Defer its
    // destruction until signal delivery has returned. Retrying with an invalid
    // configuration cannot recover, so leave the wrapper stopped in that case.
    enqueue_to_dispatcher(
        [this, client_generation, action = request.recommended_action] {
          if (!running_ ||
              client_generation_ != client_generation) {
            return;
          }

          peer_id_.reset();
          client_ = nullptr;

          if (action == xpc::client_recovery_action::correct_configuration) {
            running_ = false;
            logger::get_logger()->error("{0} stopped because its XPC configuration is invalid.",
                                        parameters_.client_name);
            return;
          }

          logger::get_logger()->debug("{0} is restarting after a terminal error.",
                                      parameters_.client_name);
          async_start();
        },
        when_now() + std::chrono::seconds(1));
  }

  void handle_peer_closed(xpc::peer_id peer_id) {
    if (peer_id_ != peer_id) {
      return;
    }

    peer_id_.reset();

    enqueue_to_dispatcher([this] {
      closed();
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
      logger::get_logger()->error("{0} received data is corrupted",
                                  parameters_.client_name);
    }
  }

  void stop() {
    running_ = false;
    ++client_generation_;

    if (!client_) {
      return;
    }

    peer_id_.reset();
    client_ = nullptr;

    logger::get_logger()->debug("{0} is stopped.",
                                parameters_.client_name);
  }

  parameters parameters_;
  std::unique_ptr<xpc::client> client_;
  std::optional<xpc::peer_id> peer_id_;
  std::uint64_t client_generation_{0};
  bool running_{false};
  std::once_flag unregister_callbacks_and_detach_once_;
};
} // namespace krbn
