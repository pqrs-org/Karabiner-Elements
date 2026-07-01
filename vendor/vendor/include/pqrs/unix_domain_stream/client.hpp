#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::unix_domain_stream::client` can be used safely in a multi-threaded environment.

#include "impl/credentials.hpp"
#include "impl/peer.hpp"
#include "impl/request_manager.hpp"
#include "options.hpp"
#include "peer_credentials.hpp"
#include "types.hpp"
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <vector>

namespace pqrs::unix_domain_stream {

[[nodiscard]] inline bool default_client_verify_peer(const peer_credentials&) noexcept {
  return true;
}

class client final : public dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(const peer_credentials&)> connected;
  nod::signal<void(const peer_credentials&)> peer_verification_failed;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(const asio::error_code&)> error_occurred;
  nod::signal<void(not_null_shared_ptr_t<std::vector<uint8_t>>)> received;
  nod::signal<void(request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> request_received;

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& socket_file_path,
         const client_options& options = {},
         std::function<bool(const peer_credentials&)> verify_peer = default_client_verify_peer)
      : dispatcher_client(weak_dispatcher),
        socket_file_path_(socket_file_path),
        options_(options),
        verify_peer_(verify_peer),
        reconnect_task_(*this),
        request_manager_(io_ctx_,
                         *this),
        work_guard_(asio::make_work_guard(io_ctx_)) {
    io_ctx_thread_ = std::thread([this] {
      io_ctx_.run();
    });
  }

  ~client() override {
    detach_from_dispatcher([this] {
      stop();
    });

    asio::post(
        io_ctx_,
        [this] {
          if (peer_) {
            peer_->async_close();
            peer_.reset();
          }
          work_guard_.reset();
        });

    if (io_ctx_thread_.joinable()) {
      io_ctx_thread_.join();
    }
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      stopped_ = false;
      connect();
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_invalidate_connection() {
    enqueue_to_dispatcher([this] {
      invalidate_connection();
    });
  }

  void async_send(const std::vector<uint8_t>& data) {
    asio::post(
        io_ctx_,
        [this, data] {
          if (peer_) {
            peer_->async_send(data);
          }
        });
  }

  void async_respond(request_id request_id_value,
                     const std::vector<uint8_t>& data) {
    asio::post(
        io_ctx_,
        [this, request_id_value, data] {
          if (peer_) {
            peer_->async_send_response(request_id_value,
                                       data);
          }
        });
  }

  void async_request(const std::vector<uint8_t>& data,
                     async_request_callback callback) {
    async_request(data,
                  options_.read_timeout,
                  callback);
  }

  void async_request(const std::vector<uint8_t>& data,
                     std::chrono::milliseconds timeout,
                     async_request_callback callback) {
    asio::post(
        io_ctx_,
        [this, data, timeout, callback] {
          if (!peer_) {
            enqueue_to_dispatcher([callback] {
              callback(asio::error::not_connected,
                       nullptr);
            });
            return;
          }

          send_request(data,
                       timeout,
                       callback);
        });
  }

private:
  // This method is executed in the dispatcher thread.
  void stop() {
    stopped_ = true;
    reconnect_task_.cancel();

    asio::post(
        io_ctx_,
        [this] {
          close_connecting_socket();
          close_peer(asio::error::operation_aborted);
        });
  }

  // This method is executed in the dispatcher thread.
  void connect() {
    asio::post(
        io_ctx_,
        [this] {
          if (stopped_ ||
              connecting_socket_ ||
              peer_) {
            return;
          }

          not_null_shared_ptr_t<asio::local::stream_protocol::socket> socket(std::make_shared<asio::local::stream_protocol::socket>(io_ctx_));
          connecting_socket_ = socket;

          socket->async_connect(
              asio::local::stream_protocol::endpoint(socket_file_path_),
              [this, socket](auto&& error_code) mutable {
                // A newer connect attempt or invalidate_connection has replaced this socket.
                if (stopped_ ||
                    connecting_socket_ != socket.get()) {
                  asio::error_code close_error_code;
                  socket->close(close_error_code);
                  return;
                }

                if (error_code) {
                  connecting_socket_.reset();

                  enqueue_to_dispatcher([this, error_code] {
                    connect_failed(error_code);
                    schedule_reconnect();
                  });
                  return;
                }

                auto credentials = impl::make_peer_credentials(*socket);

                if (!enqueue_to_dispatcher([this, socket, credentials] {
                      auto verified = verify_peer_(credentials);

                      asio::post(
                          io_ctx_,
                          [this, socket, credentials, verified] {
                            handle_connected_socket(socket,
                                                    credentials,
                                                    verified);
                          });
                    })) {
                  connecting_socket_.reset();

                  asio::error_code close_error_code;
                  socket->close(close_error_code);
                }
              });
        });
  }

  // This method is executed in the dispatcher thread.
  void invalidate_connection() {
    reconnect_task_.cancel();

    asio::post(
        io_ctx_,
        [this] {
          close_connecting_socket();
          close_peer(asio::error::operation_aborted);

          enqueue_to_dispatcher([this] {
            schedule_reconnect();
          });
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void close_connecting_socket() {
    if (connecting_socket_) {
      asio::error_code close_error_code;
      connecting_socket_->close(close_error_code);
      connecting_socket_.reset();
    }
  }

  // This method is executed in `io_ctx_thread_`.
  void handle_connected_socket(not_null_shared_ptr_t<asio::local::stream_protocol::socket> socket,
                               const peer_credentials& credentials,
                               bool verified) {
    // A newer connect attempt or invalidate_connection has replaced this socket.
    if (connecting_socket_ != socket.get()) {
      asio::error_code close_error_code;
      socket->close(close_error_code);
      return;
    }

    connecting_socket_.reset();

    if (stopped_) {
      asio::error_code close_error_code;
      socket->close(close_error_code);
      return;
    }

    if (!verified) {
      asio::error_code close_error_code;
      socket->close(close_error_code);

      enqueue_to_dispatcher([this, credentials] {
        peer_verification_failed(credentials);
        schedule_reconnect();
      });
      return;
    }

    not_null_shared_ptr_t<impl::peer> p(std::make_shared<impl::peer>(weak_dispatcher_,
                                                                     std::move(*socket),
                                                                     options_));
    peer_ = p;
    auto weak_p = make_weak(p);

    p->received.connect([this, weak_p](auto&& buffer) {
      if (auto p = weak_p.lock()) {
        asio::post(
            io_ctx_,
            [this, p, buffer] {
              if (peer_ != p) {
                return;
              }

              enqueue_to_dispatcher([this, buffer] {
                received(buffer);
              });
            });
      }
    });

    p->request_received.connect([this, weak_p](auto request_id, auto&& buffer) {
      if (auto p = weak_p.lock()) {
        asio::post(
            io_ctx_,
            [this, p, request_id, buffer] {
              if (peer_ != p) {
                return;
              }

              enqueue_to_dispatcher([this, request_id, buffer] {
                request_received(request_id,
                                 buffer);
              });
            });
      }
    });

    p->response_received.connect([this, weak_p](auto request_id, auto&& buffer) {
      if (auto p = weak_p.lock()) {
        asio::post(
            io_ctx_,
            [this, p, request_id, buffer] {
              if (peer_ == p) {
                request_manager_.complete(request_id,
                                          asio::error_code(),
                                          buffer);
              }
            });
      }
    });

    p->error_occurred.connect([this, weak_p](auto&& error_code) {
      if (auto p = weak_p.lock()) {
        asio::post(
            io_ctx_,
            [this, p, error_code] {
              if (peer_ != p) {
                return;
              }

              request_manager_.complete_all(error_code);

              enqueue_to_dispatcher([this, error_code] {
                error_occurred(error_code);
              });
            });
      }
    });

    p->closed.connect([this, weak_p] {
      if (auto p = weak_p.lock()) {
        asio::post(
            io_ctx_,
            [this, p] {
              if (peer_ != p) {
                return;
              }

              request_manager_.complete_all(asio::error::connection_reset);
              peer_.reset();

              enqueue_to_dispatcher([this] {
                closed();
                schedule_reconnect();
              });
            });
      }
    });

    p->async_start();

    asio::post(
        io_ctx_,
        [this, p, credentials] {
          if (peer_ == p.get()) {
            enqueue_to_dispatcher([this, credentials] {
              connected(credentials);
            });
          }
        });
  }

  // This method is executed in the dispatcher thread.
  void schedule_reconnect() {
    if (stopped_) {
      return;
    }

    reconnect_task_.debounce_after(
        [this] {
          if (stopped_) {
            return;
          }

          connect();
        },
        impl::normalize_scheduling_interval(options_.reconnect_interval));
  }

  // This method is executed in `io_ctx_thread_`.
  void send_request(const std::vector<uint8_t>& data,
                    std::chrono::milliseconds timeout,
                    async_request_callback callback) {
    if (!peer_) {
      enqueue_to_dispatcher([callback] {
        callback(asio::error::not_connected,
                 nullptr);
      });
      return;
    }

    auto id = request_manager_.add(std::nullopt,
                                   timeout,
                                   callback,
                                   [this] {
                                     if (options_.invalidate_connection_on_request_error) {
                                       if (close_peer(asio::error::connection_reset)) {
                                         enqueue_to_dispatcher([this] {
                                           closed();
                                           schedule_reconnect();
                                         });
                                       }
                                     }
                                   });

    peer_->async_send_request(id,
                              data);
  }

  // This method is executed in `io_ctx_thread_`.
  bool close_peer(const asio::error_code& pending_request_error_code) {
    if (peer_) {
      request_manager_.complete_all(pending_request_error_code);
      peer_->async_close();
      peer_.reset();

      return true;
    }

    return false;
  }

  std::filesystem::path socket_file_path_;
  client_options options_;
  std::function<bool(const peer_credentials&)> verify_peer_;
  dispatcher::extra::debounced_task reconnect_task_;
  std::atomic_bool stopped_ = true;

  asio::io_context io_ctx_;
  impl::request_manager request_manager_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread io_ctx_thread_;

  // Keeps the current async_connect attempt alive and lets stop/invalidate
  // close it. Completion handlers compare against this pointer so stale
  // connect attempts are ignored after async_invalidate_connection.
  std::shared_ptr<asio::local::stream_protocol::socket> connecting_socket_;
  std::shared_ptr<impl::peer> peer_;
};

} // namespace pqrs::unix_domain_stream
