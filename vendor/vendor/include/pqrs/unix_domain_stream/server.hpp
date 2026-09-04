#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::unix_domain_stream::server` can be used safely in a multi-threaded environment.

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
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pqrs::unix_domain_stream {

[[nodiscard]] inline bool default_verify_peer(const peer_credentials&) noexcept {
  return true;
}

namespace impl {

class server_state final : public dispatcher::extra::dispatcher_client,
                           public std::enable_shared_from_this<server_state> {
public:
  nod::signal<void()> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void()> closed;
  nod::signal<void(peer_id, const peer_credentials&)> peer_connected;
  nod::signal<void(peer_id)> peer_closed;
  nod::signal<void(peer_id, const asio::error_code&)> peer_error_occurred;
  nod::signal<void(peer_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> received;
  nod::signal<void(peer_id, request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> request_received;

  server_state(const server_state&) = delete;

  server_state(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
               const std::filesystem::path& socket_file_path,
               const server_options& options,
               std::function<bool(const peer_credentials&)> verify_peer)
      : dispatcher_client(weak_dispatcher),
        socket_file_path_(socket_file_path),
        options_(options),
        verify_peer_(verify_peer),
        bind_retry_task_(*this),
        socket_path_health_check_timer_(*this),
        io_ctx_(runtime::get_io_context()),
        request_manager_(io_ctx_,
                         *this) {
  }

  ~server_state() override {
    detach_from_dispatcher();
  }

  void async_shutdown() {
    if (shutdown_started_.exchange(true)) {
      return;
    }

    auto shared_self = shared_from_this();

    detach_from_dispatcher([this] {
      stopped_ = true;
      bind_retry_task_.cancel();
      socket_path_health_check_timer_.stop();
      exposed_peer_ids_.clear();

      bound.disconnect_all_slots();
      bind_failed.disconnect_all_slots();
      closed.disconnect_all_slots();
      peer_connected.disconnect_all_slots();
      peer_closed.disconnect_all_slots();
      peer_error_occurred.disconnect_all_slots();
      received.disconnect_all_slots();
      request_received.disconnect_all_slots();
    });

    asio::post(
        io_ctx_,
        [shared_self] {
          shared_self->close_acceptor();

          for (auto& [_, peer] : shared_self->peers_) {
            peer->disconnect_all_signal_slots();
          }
          shared_self->close_all_peers();

          if (shared_self->socket_path_health_check_peer_) {
            shared_self->socket_path_health_check_peer_->disconnect_all_signal_slots();
          }
          shared_self->close_socket_path_health_check_peer();
          shared_self->socket_path_health_check_in_progress_ = false;

          // Keep state alive through the cancellation handlers queued by the
          // acceptor, sockets, and timers closed above.
          asio::post(shared_self->io_ctx_,
                     [shared_self] {
                     });
        });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      stopped_ = false;
      bind();
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_send(peer_id id,
                  const std::vector<uint8_t>& data) {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, id, data] {
          if (auto self = weak_self.lock();
              self) {
            if (auto it = self->peers_.find(id);
                it != self->peers_.end()) {
              it->second->async_send(data);
            }
          }
        });
  }

  void async_respond(peer_id id,
                     request_id request_id_value,
                     const std::vector<uint8_t>& data) {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, id, request_id_value, data] {
          if (auto self = weak_self.lock();
              self) {
            if (auto it = self->peers_.find(id);
                it != self->peers_.end()) {
              it->second->async_send_response(request_id_value,
                                              data);
            }
          }
        });
  }

  void async_request(peer_id id,
                     const std::vector<uint8_t>& data,
                     async_request_callback callback) {
    async_request(id,
                  data,
                  options_.read_timeout,
                  callback);
  }

  void async_request(peer_id id,
                     const std::vector<uint8_t>& data,
                     std::chrono::milliseconds timeout,
                     async_request_callback callback) {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, id, data, timeout, callback] {
          auto self = weak_self.lock();
          if (!self) {
            return;
          }

          if (auto it = self->peers_.find(id);
              it != self->peers_.end()) {
            self->send_request(id,
                               it->second,
                               data,
                               timeout,
                               callback);
          } else {
            self->enqueue_to_dispatcher([callback] {
              callback(asio::error::not_connected,
                       nullptr);
            });
          }
        });
  }

  void async_close_peer(peer_id id) {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self, id] {
          if (auto self = weak_self.lock()) {
            self->close_peer(id);
          }
        });
  }

private:
  // This method is executed in the dispatcher thread.
  void stop() {
    stopped_ = true;
    bind_retry_task_.cancel();
    socket_path_health_check_timer_.stop();
    exposed_peer_ids_.clear();
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          if (auto self = weak_self.lock()) {
            self->close_acceptor();
            self->close_all_peers();
            self->close_socket_path_health_check_peer();
            self->socket_path_health_check_in_progress_ = false;
          }
        });
  }

  // This method is executed in the dispatcher thread.
  void bind() {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          auto self = weak_self.lock();
          if (!self ||
              self->stopped_ ||
              self->acceptor_) {
            return;
          }

          runtime::remove_socket_file_path(self->socket_file_path_);

          self->acceptor_ = std::make_unique<asio::local::stream_protocol::acceptor>(self->io_ctx_);

          asio::error_code error_code;
          self->acceptor_->open(asio::local::stream_protocol::endpoint(self->socket_file_path_).protocol(),
                                error_code);
          if (error_code) {
            self->handle_bind_failed(error_code);
            return;
          }

          self->acceptor_->bind(asio::local::stream_protocol::endpoint(self->socket_file_path_),
                                error_code);
          if (error_code) {
            self->handle_bind_failed(error_code);
            return;
          }

          runtime::set_socket_file_path_owner(self->socket_file_path_,
                                              self.get());

          self->acceptor_->listen(asio::socket_base::max_listen_connections,
                                  error_code);
          if (error_code) {
            self->handle_bind_failed(error_code);
            return;
          }

          self->enqueue_to_dispatcher([weak_self] {
            if (auto self = weak_self.lock()) {
              self->bound();
              self->start_socket_path_health_check_timer();
            }
          });

          self->accept();
        });
  }

  // This method is executed in the shared I/O runtime thread.
  void handle_bind_failed(const asio::error_code& error_code) {
    close_acceptor();

    enqueue_to_dispatcher([this, error_code] {
      bind_failed(error_code);
      schedule_bind_retry();
    });
  }

  // This method is executed in the shared I/O runtime thread.
  void accept() {
    if (stopped_ ||
        !acceptor_) {
      return;
    }

    acceptor_->async_accept(
        [self = shared_from_this()](auto&& error_code, auto socket) {
          if (self->stopped_) {
            asio::error_code close_error_code;
            socket.close(close_error_code);
            return;
          }

          if (error_code) {
            if (error_code != asio::error::operation_aborted) {
              self->close_acceptor();

              self->enqueue_to_dispatcher([self] {
                self->closed();
                self->schedule_bind_retry();
              });
            }
            return;
          }

          self->handle_accepted_socket(std::move(socket));
        });
  }

  // This method is executed in the shared I/O runtime thread.
  void handle_accepted_socket(asio::local::stream_protocol::socket socket) {
    if (stopped_) {
      asio::error_code close_error_code;
      socket.close(close_error_code);
      return;
    }

    auto credentials = impl::make_peer_credentials(socket);
    auto id = ++next_peer_id_;
    not_null_shared_ptr_t<impl::peer> p(std::make_shared<impl::peer>(weak_dispatcher_,
                                                                     std::move(socket),
                                                                     options_));
    peers_.emplace(id,
                   p);
    auto weak_self = weak_from_this();
    auto weak_p = make_weak(p);

    p->ready.connect([weak_self, id, credentials] {
      if (auto self = weak_self.lock()) {
        self->enqueue_to_dispatcher([weak_self, id, credentials] {
          if (auto self = weak_self.lock()) {
            if (self->verify_peer_(credentials)) {
              self->exposed_peer_ids_.insert(id);
              self->peer_connected(id,
                                   credentials);
            } else {
              asio::post(
                  self->io_ctx_,
                  [weak_self, id] {
                    if (auto self = weak_self.lock()) {
                      self->close_peer(id);
                    }
                  });
            }
          }
        });
      }
    });

    p->received.connect([weak_self, id](auto&& buffer) {
      if (auto self = weak_self.lock()) {
        self->enqueue_to_dispatcher([weak_self, id, buffer] {
          if (auto self = weak_self.lock();
              self &&
              self->exposed_peer_ids_.contains(id)) {
            self->received(id,
                           buffer);
          }
        });
      }
    });

    p->request_received.connect([weak_self, id](auto request_id, auto&& buffer) {
      if (auto self = weak_self.lock()) {
        self->enqueue_to_dispatcher([weak_self, id, request_id, buffer] {
          if (auto self = weak_self.lock();
              self &&
              self->exposed_peer_ids_.contains(id)) {
            self->request_received(id,
                                   request_id,
                                   buffer);
          }
        });
      }
    });

    p->response_received.connect([weak_self, id, weak_p](auto request_id, auto&& buffer) {
      if (auto self = weak_self.lock();
          self) {
        auto weak_self = self->weak_from_this();

        asio::post(
            self->io_ctx_,
            [weak_self, id, weak_p, request_id, buffer] {
              auto self = weak_self.lock();
              auto p = weak_p.lock();
              if (!self ||
                  !p) {
                return;
              }

              if (auto it = self->peers_.find(id);
                  it != self->peers_.end() &&
                  it->second.get() == p) {
                self->request_manager_.complete(id,
                                                request_id,
                                                asio::error_code(),
                                                buffer);
              }
            });
      }
    });

    p->error_occurred.connect([weak_self, id](auto&& error_code) {
      if (auto self = weak_self.lock()) {
        asio::post(
            self->io_ctx_,
            [weak_self, id, error_code] {
              if (auto self = weak_self.lock()) {
                self->request_manager_.complete_peer(id,
                                                     error_code);
              }
            });

        self->enqueue_to_dispatcher([weak_self, id, error_code] {
          if (auto self = weak_self.lock();
              self &&
              self->exposed_peer_ids_.contains(id)) {
            self->peer_error_occurred(id,
                                      error_code);
          }
        });
      }
    });

    p->closed.connect([weak_self, id] {
      if (auto self = weak_self.lock()) {
        asio::post(
            self->io_ctx_,
            [weak_self, id] {
              if (auto self = weak_self.lock()) {
                self->request_manager_.complete_peer(id,
                                                     asio::error::connection_reset);
                self->peers_.erase(id);
              }
            });

        self->enqueue_to_dispatcher([weak_self, id] {
          if (auto self = weak_self.lock();
              self &&
              self->exposed_peer_ids_.erase(id) > 0) {
            self->peer_closed(id);
          }
        });
      }
    });

    p->async_start();

    accept();
  }

  // This method is executed in the shared I/O runtime thread.
  void close_acceptor() {
    asio::error_code error_code;
    if (acceptor_) {
      acceptor_->cancel(error_code);
      acceptor_->close(error_code);
      acceptor_.reset();
    }

    runtime::remove_socket_file_path_if_owned(socket_file_path_,
                                              this);
  }

  // This method is executed in the dispatcher thread.
  void schedule_bind_retry() {
    socket_path_health_check_timer_.stop();

    if (stopped_) {
      return;
    }

    bind_retry_task_.debounce_after(
        [this] {
          if (stopped_) {
            return;
          }

          bind();
        },
        impl::normalize_scheduling_interval(options_.bind_retry_interval));
  }

  // This method is executed in the dispatcher thread.
  void start_socket_path_health_check_timer() {
    if (stopped_) {
      return;
    }

    socket_path_health_check_timer_.start(
        [this] {
          socket_path_health_check();
        },
        impl::normalize_scheduling_interval(options_.socket_path_health_check_interval));
  }

  // This method is executed in the dispatcher thread.
  void socket_path_health_check() {
    auto weak_self = weak_from_this();

    asio::post(
        io_ctx_,
        [weak_self] {
          auto self = weak_self.lock();
          if (!self ||
              self->stopped_ ||
              !self->acceptor_ ||
              self->socket_path_health_check_in_progress_) {
            return;
          }

          self->socket_path_health_check_in_progress_ = true;

          not_null_shared_ptr_t<asio::local::stream_protocol::socket> socket(std::make_shared<asio::local::stream_protocol::socket>(self->io_ctx_));
          not_null_shared_ptr_t<asio::steady_timer> timeout(std::make_shared<asio::steady_timer>(self->io_ctx_));

          timeout->expires_after(self->options_.socket_path_health_check_timeout);
          timeout->async_wait([self, socket](const auto& error_code) {
            if (!error_code) {
              asio::error_code close_error_code;
              socket->close(close_error_code);
              self->handle_socket_path_health_check_failed(asio::error::timed_out);
            }
          });

          socket->async_connect(
              asio::local::stream_protocol::endpoint(self->socket_file_path_),
              [self, socket, timeout](auto&& error_code) mutable {
                if (self->stopped_) {
                  timeout->cancel();

                  asio::error_code close_error_code;
                  socket->close(close_error_code);
                  self->socket_path_health_check_in_progress_ = false;
                  return;
                }

                if (error_code) {
                  timeout->cancel();
                  self->handle_socket_path_health_check_failed(error_code);
                  return;
                }

                not_null_shared_ptr_t<impl::peer> p(std::make_shared<impl::peer>(self->weak_dispatcher_,
                                                                                 std::move(*socket),
                                                                                 self->options_));
                self->socket_path_health_check_peer_ = p;
                auto weak_self = self->weak_from_this();

                p->health_check_response_received.connect([weak_self, timeout] {
                  if (auto self = weak_self.lock()) {
                    asio::post(
                        self->io_ctx_,
                        [weak_self, timeout] {
                          if (auto self = weak_self.lock()) {
                            timeout->cancel();

                            self->close_socket_path_health_check_peer();

                            self->socket_path_health_check_in_progress_ = false;
                          }
                        });
                  }
                });

                p->error_occurred.connect([weak_self, timeout](auto&& error_code) {
                  if (auto self = weak_self.lock()) {
                    asio::post(
                        self->io_ctx_,
                        [weak_self, timeout, error_code] {
                          if (auto self = weak_self.lock()) {
                            timeout->cancel();
                            self->handle_socket_path_health_check_failed(error_code);
                          }
                        });
                  }
                });

                p->closed.connect([weak_self, timeout] {
                  if (auto self = weak_self.lock()) {
                    asio::post(
                        self->io_ctx_,
                        [weak_self, timeout] {
                          if (auto self = weak_self.lock()) {
                            timeout->cancel();

                            if (self->socket_path_health_check_in_progress_) {
                              self->handle_socket_path_health_check_failed(asio::error::eof);
                            }
                          }
                        });
                  }
                });

                p->async_start();
                p->async_send_health_check();
              });
        });
  }

  // This method is executed in the shared I/O runtime thread.
  void handle_socket_path_health_check_failed(const asio::error_code&) {
    if (!socket_path_health_check_in_progress_) {
      return;
    }

    socket_path_health_check_in_progress_ = false;

    close_socket_path_health_check_peer();

    close_acceptor();

    enqueue_to_dispatcher([this] {
      closed();
      schedule_bind_retry();
    });
  }

  // This method is executed in the shared I/O runtime thread.
  void close_peer(peer_id id) {
    if (auto it = peers_.find(id);
        it != peers_.end()) {
      request_manager_.complete_peer(id,
                                     asio::error::operation_aborted);
      it->second->async_close();
      peers_.erase(it);
    }
  }

  // This method is executed in the shared I/O runtime thread.
  void close_socket_path_health_check_peer() {
    if (socket_path_health_check_peer_) {
      socket_path_health_check_peer_->async_close();
      socket_path_health_check_peer_.reset();
    }
  }

  // This method is executed in the shared I/O runtime thread.
  void close_all_peers() {
    request_manager_.complete_all(asio::error::operation_aborted);

    for (auto& [_, p] : peers_) {
      p->async_close();
    }

    peers_.clear();
  }

  // This method is executed in the shared I/O runtime thread.
  void send_request(peer_id peer_id_value,
                    not_null_shared_ptr_t<impl::peer> peer,
                    const std::vector<uint8_t>& data,
                    std::chrono::milliseconds timeout,
                    async_request_callback callback) {
    auto id = request_manager_.add(peer_id_value,
                                   timeout,
                                   callback,
                                   [this, peer_id_value] {
                                     if (options_.invalidate_connection_on_request_error) {
                                       close_peer(peer_id_value);
                                     }
                                   });

    peer->async_send_request(id,
                             data);
  }

  std::filesystem::path socket_file_path_;
  server_options options_;
  std::function<bool(const peer_credentials&)> verify_peer_;
  dispatcher::extra::debounced_task bind_retry_task_;
  dispatcher::extra::timer socket_path_health_check_timer_;
  std::atomic_bool stopped_ = true;

  asio::io_context& io_ctx_;
  request_manager request_manager_;
  std::unique_ptr<asio::local::stream_protocol::acceptor> acceptor_;
  std::unordered_map<peer_id, not_null_shared_ptr_t<peer>> peers_;
  std::unordered_set<peer_id> exposed_peer_ids_;
  std::shared_ptr<peer> socket_path_health_check_peer_;
  bool socket_path_health_check_in_progress_ = false;
  peer_id next_peer_id_ = 0;
  std::atomic_bool shutdown_started_ = false;
};

} // namespace impl

// A facade that lets the public object be destroyed without waiting for the
// I/O thread while server_state remains alive until queued shutdown work ends.
class server final : public dispatcher::extra::dispatcher_client {
private:
  // This member must be declared before the signal references below because
  // members are initialized in declaration order.
  not_null_shared_ptr_t<impl::server_state> state_;

public:
  nod::signal<void()>& bound;
  nod::signal<void(const asio::error_code&)>& bind_failed;
  nod::signal<void()>& closed;
  nod::signal<void(peer_id, const peer_credentials&)>& peer_connected;
  nod::signal<void(peer_id)>& peer_closed;
  nod::signal<void(peer_id, const asio::error_code&)>& peer_error_occurred;
  nod::signal<void(peer_id, not_null_shared_ptr_t<std::vector<uint8_t>>)>& received;
  nod::signal<void(peer_id, request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)>& request_received;

  server(const server&) = delete;

  server(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& socket_file_path,
         const server_options& options = {},
         std::function<bool(const peer_credentials&)> verify_peer = default_verify_peer)
      : dispatcher_client(weak_dispatcher),
        state_(std::make_shared<impl::server_state>(weak_dispatcher,
                                                    socket_file_path,
                                                    options,
                                                    std::move(verify_peer))),
        bound(state_->bound),
        bind_failed(state_->bind_failed),
        closed(state_->closed),
        peer_connected(state_->peer_connected),
        peer_closed(state_->peer_closed),
        peer_error_occurred(state_->peer_error_occurred),
        received(state_->received),
        request_received(state_->request_received) {
  }

  ~server() override {
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

  void async_send(peer_id id,
                  const std::vector<uint8_t>& data) const {
    state_->async_send(id,
                       data);
  }

  void async_respond(peer_id id,
                     request_id request_id_value,
                     const std::vector<uint8_t>& data) const {
    state_->async_respond(id,
                          request_id_value,
                          data);
  }

  void async_request(peer_id id,
                     const std::vector<uint8_t>& data,
                     async_request_callback callback) const {
    state_->async_request(id,
                          data,
                          callback);
  }

  void async_request(peer_id id,
                     const std::vector<uint8_t>& data,
                     std::chrono::milliseconds timeout,
                     async_request_callback callback) const {
    state_->async_request(id,
                          data,
                          timeout,
                          callback);
  }

  void async_close_peer(peer_id id) const {
    state_->async_close_peer(id);
  }
};

} // namespace pqrs::unix_domain_stream
