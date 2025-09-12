#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::client_impl` can be used safely in a multi-threaded environment.

#include "asio_helper.hpp"
#include "base_impl.hpp"
#include "send_entry.hpp"
#include <deque>
#include <filesystem>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
namespace impl {
class client_impl final : public base_impl {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(std::optional<pid_t> peer_pid)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;

  // Methods

  client_impl(const client_impl&) = delete;

  client_impl(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
              std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> send_entries) : base_impl(weak_dispatcher,
                                                                                                 base_impl::mode::client,
                                                                                                 send_entries),
                                                                                       server_check_timer_(*this),
                                                                                       client_socket_check_timer_(*this),
                                                                                       client_socket_check_client_send_entries_(std::make_shared<std::deque<std::shared_ptr<impl::send_entry>>>()) {
  }

  ~client_impl(void) {
    async_close();

    terminate_base_impl();
  }

  // Set client_socket_file_path if you need bidirectional communication.
  void async_connect(const std::filesystem::path& server_socket_file_path,
                     const std::optional<std::filesystem::path>& client_socket_file_path,
                     size_t buffer_size,
                     std::optional<std::chrono::milliseconds> server_check_interval,
                     std::optional<std::chrono::milliseconds> next_heartbeat_deadline,
                     std::optional<std::chrono::milliseconds> client_socket_check_interval) {
    asio::post(io_ctx_, [this,
                         server_socket_file_path,
                         client_socket_file_path,
                         buffer_size,
                         server_check_interval,
                         next_heartbeat_deadline,
                         client_socket_check_interval] {
      if (socket_) {
        return;
      }

      socket_ = std::make_unique<asio::local::datagram_protocol::socket>(io_ctx_);
      socket_ready_ = false;

      // Remove existing file before `bind`.

      if (client_socket_file_path) {
        std::error_code error_code;
        std::filesystem::remove(*client_socket_file_path, error_code);
      }

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

        bound_path_ = *client_socket_file_path;
      }

      // Connect

      socket_->async_connect(
          asio::local::datagram_protocol::endpoint(server_socket_file_path),
          [this,
           server_check_interval,
           next_heartbeat_deadline,
           client_socket_check_interval,
           client_socket_file_path](auto&& error_code) {
            if (error_code) {
              enqueue_to_dispatcher([this, error_code] {
                connect_failed(error_code);
              });
            } else {
              socket_ready_ = true;

              stop_server_check();
              start_server_check(server_check_interval,
                                 next_heartbeat_deadline);

              stop_client_socket_check();
              start_client_socket_check(client_socket_file_path,
                                        client_socket_check_interval);

              std::optional<pid_t> peer_pid;
              if (socket_) {
                pid_t pid{};
                socklen_t len = sizeof(pid);
                if (getsockopt(socket_->native_handle(),
                               SOL_LOCAL,
                               LOCAL_PEERPID,
                               &pid,
                               &len) == 0) {
                  peer_pid = pid;
                }
              }

              enqueue_to_dispatcher([this, peer_pid] {
                connected(peer_pid);
              });

              start_actors();
            }
          });
    });
  }

private:
  // This method is executed in `io_ctx_thread_`.
  void start_server_check(std::optional<std::chrono::milliseconds> server_check_interval,
                          std::optional<std::chrono::milliseconds> next_heartbeat_deadline) {
    if (server_check_interval) {
      server_check_timer_.start(
          [this,
           next_heartbeat_deadline] {
            asio::post(io_ctx_, [this,
                                 next_heartbeat_deadline] {
              check_server(next_heartbeat_deadline);
            });
          },
          *server_check_interval);
    }
  }

  // This method is executed in `io_ctx_thread_`.
  void stop_server_check(void) {
    server_check_timer_.stop();
  }

  // This method is executed in `io_ctx_thread_`.
  void check_server(std::optional<std::chrono::milliseconds> next_heartbeat_deadline) {
    if (!socket_ ||
        !socket_ready_) {
      stop_server_check();
    }

    uint32_t next_heartbeat_deadline_value = 0;
    if (next_heartbeat_deadline) {
      next_heartbeat_deadline_value = next_heartbeat_deadline->count();
    }

    std::vector<uint8_t> v(sizeof(uint32_t));
    auto p = reinterpret_cast<uint32_t*>(&(v[0]));
    *p++ = next_heartbeat_deadline_value;

    auto b = std::make_shared<send_entry>(send_entry::type::heartbeat,
                                          v,
                                          nullptr);
    async_send(b);
  }

  // This method is executed in `io_ctx_thread_`.
  void start_client_socket_check(std::optional<std::filesystem::path> client_socket_file_path,
                                 std::optional<std::chrono::milliseconds> client_socket_check_interval) {
    if (client_socket_file_path &&
        client_socket_check_interval) {
      client_socket_check_timer_.start(
          [this, client_socket_file_path] {
            asio::post(io_ctx_, [this, client_socket_file_path] {
              check_client_socket(*client_socket_file_path);
            });
          },
          *client_socket_check_interval);
    }
  }

  // This method is executed in `io_ctx_thread_`.
  void stop_client_socket_check(void) {
    client_socket_check_timer_.stop();
    client_socket_check_client_impl_ = nullptr;
  }

  // This method is executed in `io_ctx_thread_`.
  void check_client_socket(const std::filesystem::path& client_socket_file_path) {
    if (!socket_ ||
        !socket_ready_) {
      stop_client_socket_check();
    }

    if (!client_socket_check_client_impl_) {
      client_socket_check_client_impl_ = std::make_unique<client_impl>(
          weak_dispatcher_,
          client_socket_check_client_send_entries_);

      client_socket_check_client_impl_->connected.connect([this](auto&& peer_pid) {
        asio::post(io_ctx_, [this] {
          client_socket_check_client_impl_ = nullptr;
        });
      });

      client_socket_check_client_impl_->connect_failed.connect([this](auto&& error_code) {
        async_close();
      });

      size_t buffer_size = 32;
      client_socket_check_client_impl_->async_connect(client_socket_file_path,
                                                      std::nullopt, // client_socket_file_path
                                                      buffer_size,
                                                      std::nullopt, // server_check_interval
                                                      std::nullopt, // next_heartbeat_deadline
                                                      std::nullopt  // client_socket_check_interval
      );
    }
  }

  dispatcher::extra::timer server_check_timer_;
  dispatcher::extra::timer client_socket_check_timer_;
  std::unique_ptr<client_impl> client_socket_check_client_impl_;
  std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> client_socket_check_client_send_entries_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
