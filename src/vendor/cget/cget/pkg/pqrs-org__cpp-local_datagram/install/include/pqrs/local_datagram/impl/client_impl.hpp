#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::client_impl` can be used safely in a multi-threaded environment.

#include "asio_helper.hpp"
#include "base_impl.hpp"
#include "send_entry.hpp"
#include <deque>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
namespace impl {
class client_impl final : public base_impl {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;

  // Methods

  client_impl(const client_impl&) = delete;

  client_impl(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
              std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> send_entries) : base_impl(weak_dispatcher,
                                                                                                 base_impl::mode::client,
                                                                                                 send_entries),
                                                                                       server_check_timer_(*this) {
  }

  ~client_impl(void) {
    async_close();

    terminate_base_impl();
  }

  // Set client_socket_file_path if you need bidirectional communication.
  void async_connect(const std::string& server_socket_file_path,
                     const std::optional<std::string>& client_socket_file_path,
                     size_t buffer_size,
                     std::optional<std::chrono::milliseconds> server_check_interval) {
    io_service_.post([this, server_socket_file_path, client_socket_file_path, buffer_size, server_check_interval] {
      if (socket_) {
        return;
      }

      socket_ = std::make_unique<asio::local::datagram_protocol::socket>(io_service_);
      socket_ready_ = false;

      // Open

      {
        asio::error_code error_code;
        socket_->open(asio::local::datagram_protocol::socket::protocol_type(),
                      error_code);
        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            connect_failed(error_code);
          });
          return;
        }
      }

      set_socket_options(buffer_size);

      // Bind

      if (client_socket_file_path) {
        asio::error_code error_code;
        socket_->bind(asio::local::datagram_protocol::endpoint(*client_socket_file_path),
                      error_code);

        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            bind_failed(error_code);
          });
          return;
        }
      }

      // Connect

      socket_->async_connect(asio::local::datagram_protocol::endpoint(server_socket_file_path),
                             [this, server_check_interval](auto&& error_code) {
                               if (error_code) {
                                 enqueue_to_dispatcher([this, error_code] {
                                   connect_failed(error_code);
                                 });
                               } else {
                                 socket_ready_ = true;

                                 stop_server_check();
                                 start_server_check(server_check_interval);

                                 enqueue_to_dispatcher([this] {
                                   connected();
                                 });

                                 start_actors();
                               }
                             });
    });
  }

private:
  // This method is executed in `io_service_thread_`.
  void start_server_check(std::optional<std::chrono::milliseconds> server_check_interval) {
    if (server_check_interval) {
      server_check_timer_.start(
          [this] {
            io_service_.post([this] {
              check_server();
            });
          },
          *server_check_interval);
    }
  }

  // This method is executed in `io_service_thread_`.
  void stop_server_check(void) {
    server_check_timer_.stop();
  }

  // This method is executed in `io_service_thread_`.
  void check_server(void) {
    if (!socket_ ||
        !socket_ready_) {
      stop_server_check();
    }

    auto b = std::make_shared<send_entry>(send_entry::type::server_check, nullptr);
    async_send(b);
  }

  dispatcher::extra::timer server_check_timer_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
