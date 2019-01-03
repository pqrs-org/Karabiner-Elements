#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::client` can be used safely in a multi-threaded environment.

#ifdef ASIO_STANDALONE
#include <asio.hpp>
#else
#define ASIO_STANDALONE
#include <asio.hpp>
#undef ASIO_STANDALONE
#endif

#include "buffer.hpp"
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
namespace impl {
class client final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                  io_service_(),
                                                                  work_(std::make_unique<asio::io_service::work>(io_service_)),
                                                                  socket_(io_service_),
                                                                  connected_(false),
                                                                  server_check_timer_(*this) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  virtual ~client(void) {
    async_close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }

    detach_from_dispatcher();
  }

  void async_connect(const std::string& path,
                     std::optional<std::chrono::milliseconds> server_check_interval) {
    async_close();

    io_service_.post([this, path, server_check_interval] {
      connected_ = false;

      // Open

      {
        asio::error_code error_code;
        socket_.open(asio::local::datagram_protocol::socket::protocol_type(),
                     error_code);
        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            connect_failed(error_code);
          });
          return;
        }
      }

      // Connect

      socket_.async_connect(asio::local::datagram_protocol::endpoint(path),
                            [this, server_check_interval](auto&& error_code) {
                              if (error_code) {
                                enqueue_to_dispatcher([this, error_code] {
                                  connect_failed(error_code);
                                });
                              } else {
                                connected_ = true;

                                stop_server_check();
                                start_server_check(server_check_interval);

                                enqueue_to_dispatcher([this] {
                                  connected();
                                });
                              }
                            });
    });
  }

  void async_close(void) {
    io_service_.post([this] {
      stop_server_check();

      // Close socket

      asio::error_code error_code;

      socket_.cancel(error_code);
      socket_.close(error_code);

      // Signal

      if (connected_) {
        connected_ = false;

        enqueue_to_dispatcher([this] {
          closed();
        });
      }
    });
  }

  void async_send(std::shared_ptr<buffer> buffer) {
    io_service_.post([this, buffer] {
      socket_.async_send(asio::buffer(buffer->get_vector()),
                         [this, buffer](auto&& error_code,
                                        auto&& bytes_transferred) {
                           if (error_code) {
                             async_close();
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
    auto b = std::make_shared<buffer>();
    async_send(b);
  }

  asio::io_service io_service_;
  std::unique_ptr<asio::io_service::work> work_;
  asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool connected_;

  dispatcher::extra::timer server_check_timer_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
