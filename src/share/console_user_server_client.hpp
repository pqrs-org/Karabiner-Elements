#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(void) : dispatcher_client() {
  }

  virtual ~console_user_server_client(void) {
    detach_from_dispatcher([this] {
      client_ = nullptr;
    });
  }

  void async_start(const std::string& endpoint_path) {
    enqueue_to_dispatcher([this, endpoint_path] {
      client_ = nullptr;

      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               endpoint_path,
                                                               std::nullopt,
                                                               constants::local_datagram_buffer_size);
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_next_heartbeat_deadline(std::chrono::milliseconds(10000));
      client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));

      client_->connected.connect([this, endpoint_path](auto&& peer_pid) {
        logger::get_logger()->info("console_user_server_client is connected: {0}", endpoint_path);

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->closed.connect([this, endpoint_path] {
        logger::get_logger()->info("console_user_server_client is closed: {0}", endpoint_path);

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client error: {0}", error_code.message());
      });

      client_->async_start();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      client_ = nullptr;
    });
  }

private:
  std::unique_ptr<pqrs::local_datagram::client> client_;
};
} // namespace krbn
