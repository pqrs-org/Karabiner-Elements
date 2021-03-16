#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::server_impl` can be used safely in a multi-threaded environment.

#include "base_impl.hpp"
#include "client_impl.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <unistd.h>

namespace pqrs {
namespace local_datagram {
namespace impl {
class server_impl final : public base_impl {
public:
  // Methods

  server_impl(const server_impl&) = delete;

  server_impl(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
              std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> send_entries) : base_impl(weak_dispatcher,
                                                                                                 base_impl::mode::server,
                                                                                                 send_entries),
                                                                                       server_check_timer_(*this),
                                                                                       server_check_client_send_entries_(std::make_shared<std::deque<std::shared_ptr<impl::send_entry>>>()) {
  }

  ~server_impl(void) {
    async_close();

    terminate_base_impl();
  }

  void async_bind(const std::filesystem::path& server_socket_file_path,
                  size_t buffer_size,
                  std::optional<std::chrono::milliseconds> server_check_interval) {
    async_close();

    io_service_.post([this, server_socket_file_path, buffer_size, server_check_interval] {
      socket_ready_ = false;

      // Remove existing file before `bind`.

      {
        std::error_code error_code;
        std::filesystem::remove(server_socket_file_path, error_code);
      }

      // Open

      socket_ = std::make_unique<asio::local::datagram_protocol::socket>(io_service_);

      {
        asio::error_code error_code;
        socket_->open(asio::local::datagram_protocol::socket::protocol_type(),
                      error_code);
        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            bind_failed(error_code);
          });
          return;
        }
      }

      set_socket_options(buffer_size);

      // Bind

      {
        asio::error_code error_code;
        socket_->bind(asio::local::datagram_protocol::endpoint(server_socket_file_path),
                      error_code);

        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            bind_failed(error_code);
          });
          return;
        }

        bound_path_ = server_socket_file_path;
      }

      // Signal

      socket_ready_ = true;

      start_server_check(server_socket_file_path,
                         server_check_interval);

      enqueue_to_dispatcher([this] {
        bound();
      });

      start_actors();
    });
  }

private:
  // This method is executed in `io_service_thread_`.
  void start_server_check(const std::filesystem::path& server_socket_file_path,
                          std::optional<std::chrono::milliseconds> server_check_interval) {
    if (server_check_interval) {
      server_check_timer_.start(
          [this, server_socket_file_path] {
            io_service_.post([this, server_socket_file_path] {
              check_server(server_socket_file_path);
            });
          },
          *server_check_interval);
    }
  }

  // This method is executed in `io_service_thread_`.
  void stop_server_check(void) {
    server_check_timer_.stop();
    server_check_client_impl_ = nullptr;
  }

  // This method is executed in `io_service_thread_`.
  void check_server(const std::filesystem::path& server_socket_file_path) {
    if (!socket_ ||
        !socket_ready_) {
      stop_server_check();
    }

    if (!server_check_client_impl_) {
      server_check_client_impl_ = std::make_unique<client_impl>(
          weak_dispatcher_,
          server_check_client_send_entries_);

      server_check_client_impl_->connected.connect([this] {
        io_service_.post([this] {
          server_check_client_impl_ = nullptr;
        });
      });

      server_check_client_impl_->connect_failed.connect([this](auto&& error_code) {
        async_close();
      });

      size_t buffer_size = 32;
      server_check_client_impl_->async_connect(server_socket_file_path,
                                               std::nullopt,
                                               buffer_size,
                                               std::nullopt);
    }
  }

  dispatcher::extra::timer server_check_timer_;
  std::unique_ptr<client_impl> server_check_client_impl_;
  std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> server_check_client_send_entries_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
