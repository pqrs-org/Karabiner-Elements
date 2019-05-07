#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::server` can be used safely in a multi-threaded environment.

#include "client.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <unistd.h>

namespace pqrs {
namespace local_datagram {
namespace impl {
class server final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>)> received;

  // Methods

  server(const server&) = delete;

  server(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                  io_service_(),
                                                                  work_(std::make_unique<asio::io_service::work>(io_service_)),
                                                                  socket_(std::make_unique<asio::local::datagram_protocol::socket>(io_service_)),
                                                                  bound_(false),
                                                                  server_check_timer_(*this) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  virtual ~server(void) {
    async_close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;

      io_service_thread_.join();
    }

    detach_from_dispatcher();
  }

  void async_bind(const std::string& path,
                  size_t buffer_size,
                  std::optional<std::chrono::milliseconds> server_check_interval) {
    async_close();

    io_service_.post([this, path, buffer_size, server_check_interval] {
      bound_ = false;
      bound_path_.clear();

      // Open

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

      // Bind

      {
        asio::error_code error_code;
        socket_->bind(asio::local::datagram_protocol::endpoint(path),
                      error_code);

        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            bind_failed(error_code);
          });
          return;
        }
      }

      // Signal

      bound_ = true;
      bound_path_ = path;

      start_server_check(path,
                         server_check_interval);

      enqueue_to_dispatcher([this] {
        bound();
      });

      // Receive

      buffer_.resize(buffer_size);
      async_receive();
    });
  }

  void async_close(void) {
    io_service_.post([this] {
      close();
    });
  }

private:
  // This method is executed in `io_service_thread_`.
  void close(void) {
    stop_server_check();

    // Close socket

    asio::error_code error_code;

    socket_->cancel(error_code);
    socket_->close(error_code);

    socket_ = std::make_unique<asio::local::datagram_protocol::socket>(io_service_);

    // Signal

    if (bound_) {
      bound_ = false;
      unlink(bound_path_.c_str());

      enqueue_to_dispatcher([this] {
        closed();
      });
    }
  }

  void async_receive(void) {
    socket_->async_receive(asio::buffer(buffer_),
                           [this](auto&& error_code, auto&& bytes_transferred) {
                             if (!error_code) {
                               auto v = std::make_shared<std::vector<uint8_t>>(bytes_transferred);
                               std::copy(std::begin(buffer_),
                                         std::begin(buffer_) + bytes_transferred,
                                         std::begin(*v));
                               enqueue_to_dispatcher([this, v] {
                                 received(v);
                               });
                             }

                             // receive once if not closed

                             if (bound_) {
                               async_receive();
                             }
                           });
  }

  // This method is executed in `io_service_thread_`.
  void start_server_check(const std::string& path,
                          std::optional<std::chrono::milliseconds> server_check_interval) {
    if (server_check_interval) {
      server_check_timer_.start(
          [this, path] {
            io_service_.post([this, path] {
              check_server(path);
            });
          },
          *server_check_interval);
    }
  }

  // This method is executed in `io_service_thread_`.
  void stop_server_check(void) {
    server_check_timer_.stop();
    server_check_client_ = nullptr;
  }

  // This method is executed in `io_service_thread_`.
  void check_server(const std::string& path) {
    if (!server_check_client_) {
      server_check_client_ = std::make_unique<client>(weak_dispatcher_);

      server_check_client_->connected.connect([this] {
        io_service_.post([this] {
          server_check_client_ = nullptr;
        });
      });

      server_check_client_->connect_failed.connect([this](auto&& error_code) {
        io_service_.post([this] {
          close();
        });
      });

      server_check_client_->async_connect(path, std::nullopt);
    }
  }

  asio::io_service io_service_;
  std::unique_ptr<asio::io_service::work> work_;
  std::unique_ptr<asio::local::datagram_protocol::socket> socket_;
  std::thread io_service_thread_;
  bool bound_;
  std::string bound_path_;

  std::vector<uint8_t> buffer_;

  dispatcher::extra::timer server_check_timer_;
  std::unique_ptr<client> server_check_client_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
