#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::unix_domain_stream::client` can be used safely in a multi-threaded environment.

#include "impl/credentials.hpp"
#include "impl/peer.hpp"
#include "impl/request_manager.hpp"
#include "impl/runtime.hpp"
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

namespace impl {

class client_state final : public dispatcher::extra::dispatcher_client,
                           public std::enable_shared_from_this<client_state> {
public:
  nod::signal<void(const peer_credentials&)> connected;
  nod::signal<void(const peer_credentials&)> peer_verification_failed;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(const asio::error_code&)> error_occurred;
  nod::signal<void(not_null_shared_ptr_t<std::vector<uint8_t>>)> received;
  nod::signal<void(request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> request_received;

  client_state(const client_state&) = delete;

  client_state(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
               const std::filesystem::path& socket_file_path,
               const client_options& options,
               std::function<bool(const peer_credentials&)> verify_peer)
      : dispatcher_client(weak_dispatcher),
        socket_file_path_(socket_file_path),
        options_(options),
        verify_peer_(verify_peer),
        reconnect_task_(*this),
        io_ctx_(runtime::get_io_context()),
        request_manager_(io_ctx_,
                         *this) {
  }

  ~client_state() override {
    detach_from_dispatcher();
  }

  void async_shutdown() {
    if (shutdown_started_.exchange(true)) {
      return;
    }

    auto shared_self = shared_from_this();

    detach_from_dispatcher([this] {
      stopped_ = true;
      reconnect_task_.cancel();

      connected.disconnect_all_slots();
      peer_verification_failed.disconnect_all_slots();
      connect_failed.disconnect_all_slots();
      closed.disconnect_all_slots();
      error_occurred.disconnect_all_slots();
      received.disconnect_all_slots();
      request_received.disconnect_all_slots();
    });

    asio::post(
        io_ctx_,
        [shared_self] {
          shared_self->close_connecting_socket();

          if (shared_self->peer_) {
            shared_self->peer_->disconnect_all_signal_slots();
          }

          shared_self->close_peer(asio::error::operation_aborted);

          // Keep state alive through the cancellation handlers queued by the
          // socket and timers closed above.
          asio::post(shared_self->io_ctx_,
                     [shared_self] {
                     });
        });
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
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, data] {
          if (auto self = weak_self.lock();
              self &&
              self->peer_) {
            self->peer_->async_send(data);
          }
        });
  }

  void async_respond(request_id request_id_value,
                     const std::vector<uint8_t>& data) {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, request_id_value, data] {
          if (auto self = weak_self.lock();
              self &&
              self->peer_) {
            self->peer_->async_send_response(request_id_value,
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
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, data, timeout, callback] {
          auto self = weak_self.lock();
          if (!self) {
            return;
          }

          if (!self->peer_) {
            self->enqueue_to_dispatcher([callback] {
              callback(asio::error::not_connected,
                       nullptr);
            });
            return;
          }

          self->send_request(data,
                             timeout,
                             callback);
        });
  }

private:
  // This method is executed in the dispatcher thread.
  void stop() {
    stopped_ = true;
    reconnect_task_.cancel();
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          if (auto self = weak_self.lock()) {
            self->close_connecting_socket();
            self->close_peer(asio::error::operation_aborted);
          }
        });
  }

  // This method is executed in the dispatcher thread.
  void connect() {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          auto self = weak_self.lock();
          if (!self ||
              self->stopped_ ||
              self->connecting_socket_ ||
              self->peer_) {
            return;
          }

          not_null_shared_ptr_t<asio::local::stream_protocol::socket> socket(std::make_shared<asio::local::stream_protocol::socket>(self->io_ctx_));
          self->connecting_socket_ = socket;

          socket->async_connect(
              asio::local::stream_protocol::endpoint(self->socket_file_path_),
              [self, socket](auto&& error_code) mutable {
                // A newer connect attempt or invalidate_connection has replaced this socket.
                if (self->stopped_ ||
                    self->connecting_socket_ != socket.get()) {
                  asio::error_code close_error_code;
                  socket->close(close_error_code);
                  return;
                }

                if (error_code) {
                  self->connecting_socket_.reset();

                  self->enqueue_to_dispatcher([self, error_code] {
                    self->connect_failed(error_code);
                    self->schedule_reconnect();
                  });
                  return;
                }

                auto credentials = impl::make_peer_credentials(*socket);

                if (!self->enqueue_to_dispatcher([self, socket, credentials] {
                      auto verified = self->verify_peer_(credentials);

                      asio::post(
                          self->io_ctx_,
                          [self, socket, credentials, verified] {
                            self->handle_connected_socket(socket,
                                                          credentials,
                                                          verified);
                          });
                    })) {
                  self->connecting_socket_.reset();

                  asio::error_code close_error_code;
                  socket->close(close_error_code);
                }
              });
        });
  }

  // This method is executed in the dispatcher thread.
  void invalidate_connection() {
    reconnect_task_.cancel();
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          if (auto self = weak_self.lock()) {
            self->close_connecting_socket();
            self->close_peer(asio::error::operation_aborted);

            self->enqueue_to_dispatcher([weak_self] {
              if (auto self = weak_self.lock()) {
                self->schedule_reconnect();
              }
            });
          }
        });
  }

  // This method is executed in the shared I/O runtime thread.
  void close_connecting_socket() {
    if (connecting_socket_) {
      asio::error_code close_error_code;
      connecting_socket_->close(close_error_code);
      connecting_socket_.reset();
    }
  }

  // This method is executed in the shared I/O runtime thread.
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
    auto weak_self = weak_from_this();
    auto weak_p = make_weak(p);

    p->received.connect([weak_self, weak_p](auto&& buffer) {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, weak_p, buffer] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (!self ||
                  !p ||
                  self->peer_ != p) {
                return;
              }

              self->enqueue_to_dispatcher([weak_self, buffer] {
                if (auto self = weak_self.lock()) {
                  self->received(buffer);
                }
              });
            });
      }
    });

    p->request_received.connect([weak_self, weak_p](auto request_id, auto&& buffer) {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, weak_p, request_id, buffer] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (!self ||
                  !p ||
                  self->peer_ != p) {
                return;
              }

              self->enqueue_to_dispatcher([weak_self, request_id, buffer] {
                if (auto self = weak_self.lock()) {
                  self->request_received(request_id,
                                         buffer);
                }
              });
            });
      }
    });

    p->response_received.connect([weak_self, weak_p](auto request_id, auto&& buffer) {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, weak_p, request_id, buffer] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (self &&
                  p &&
                  self->peer_ == p) {
                self->request_manager_.complete(request_id,
                                                asio::error_code(),
                                                buffer);
              }
            });
      }
    });

    p->error_occurred.connect([weak_self, weak_p](auto&& error_code) {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, weak_p, error_code] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (!self ||
                  !p ||
                  self->peer_ != p) {
                return;
              }

              self->request_manager_.complete_all(error_code);

              self->enqueue_to_dispatcher([weak_self, error_code] {
                if (auto self = weak_self.lock()) {
                  self->error_occurred(error_code);
                }
              });
            });
      }
    });

    p->closed.connect([weak_self, weak_p] {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, weak_p] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (!self ||
                  !p ||
                  self->peer_ != p) {
                return;
              }

              self->request_manager_.complete_all(asio::error::connection_reset);
              self->peer_.reset();

              self->enqueue_to_dispatcher([weak_self] {
                if (auto self = weak_self.lock()) {
                  self->closed();
                  self->schedule_reconnect();
                }
              });
            });
      }
    });

    p->async_start();

    asio::post(
        io_ctx_,
        [weak_self, weak_p, credentials] {
          auto self = weak_self.lock();
          auto p = weak_p.lock();
          if (self &&
              p &&
              self->peer_ == p) {
            self->enqueue_to_dispatcher([weak_self, credentials] {
              if (auto self = weak_self.lock()) {
                self->connected(credentials);
              }
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

  // This method is executed in the shared I/O runtime thread.
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

  // This method is executed in the shared I/O runtime thread.
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

  asio::io_context& io_ctx_;
  request_manager request_manager_;

  // Keeps the current async_connect attempt alive and lets stop/invalidate
  // close it. Completion handlers compare against this pointer so stale
  // connect attempts are ignored after async_invalidate_connection.
  std::shared_ptr<asio::local::stream_protocol::socket> connecting_socket_;
  std::shared_ptr<peer> peer_;
  std::atomic_bool shutdown_started_ = false;
};

} // namespace impl

// A facade that lets the public object be destroyed without waiting for the
// I/O thread while client_state remains alive until queued shutdown work ends.
class client final : public dispatcher::extra::dispatcher_client {
private:
  // This member must be declared before the signal references below because
  // members are initialized in declaration order.
  not_null_shared_ptr_t<impl::client_state> state_;

public:
  nod::signal<void(const peer_credentials&)>& connected;
  nod::signal<void(const peer_credentials&)>& peer_verification_failed;
  nod::signal<void(const asio::error_code&)>& connect_failed;
  nod::signal<void()>& closed;
  nod::signal<void(const asio::error_code&)>& error_occurred;
  nod::signal<void(not_null_shared_ptr_t<std::vector<uint8_t>>)>& received;
  nod::signal<void(request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)>& request_received;

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& socket_file_path,
         const client_options& options = {},
         std::function<bool(const peer_credentials&)> verify_peer = default_client_verify_peer)
      : dispatcher_client(weak_dispatcher),
        state_(std::make_shared<impl::client_state>(weak_dispatcher,
                                                    socket_file_path,
                                                    options,
                                                    std::move(verify_peer))),
        connected(state_->connected),
        peer_verification_failed(state_->peer_verification_failed),
        connect_failed(state_->connect_failed),
        closed(state_->closed),
        error_occurred(state_->error_occurred),
        received(state_->received),
        request_received(state_->request_received) {
  }

  ~client() override {
    detach_from_dispatcher([this] {
      state_->async_shutdown();
    });
  }

  void async_start() const {
    state_->async_start();
  }

  void async_stop() const {
    state_->async_stop();
  }

  void async_invalidate_connection() const {
    state_->async_invalidate_connection();
  }

  void async_send(const std::vector<uint8_t>& data) const {
    state_->async_send(data);
  }

  void async_respond(request_id request_id_value,
                     const std::vector<uint8_t>& data) const {
    state_->async_respond(request_id_value,
                          data);
  }

  void async_request(const std::vector<uint8_t>& data,
                     async_request_callback callback) const {
    state_->async_request(data,
                          callback);
  }

  void async_request(const std::vector<uint8_t>& data,
                     std::chrono::milliseconds timeout,
                     async_request_callback callback) const {
    state_->async_request(data,
                          timeout,
                          callback);
  }
};

} // namespace pqrs::unix_domain_stream
