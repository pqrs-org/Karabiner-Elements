#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::unix_domain_stream::server` can be used safely in a multi-threaded environment.

#include "impl/credentials.hpp"
#include "impl/peer.hpp"
#include "options.hpp"
#include "peer_credentials.hpp"
#include "types.hpp"
#include <atomic>
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pqrs::unix_domain_stream {

inline bool default_verify_peer(const peer_credentials&) {
  return true;
}

class server final : public dispatcher::extra::dispatcher_client {
public:
  nod::signal<void()> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void()> closed;
  nod::signal<void(peer_id, const peer_credentials&)> peer_connected;
  nod::signal<void(peer_id)> peer_closed;
  nod::signal<void(peer_id, const asio::error_code&)> peer_error_occurred;
  nod::signal<void(peer_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> received;
  nod::signal<void(peer_id, request_id, not_null_shared_ptr_t<std::vector<uint8_t>>)> request_received;

  server(const server&) = delete;

  server(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& socket_file_path,
         const server_options& options = {},
         std::function<bool(const peer_credentials&)> verify_peer = default_verify_peer)
      : dispatcher_client(weak_dispatcher),
        socket_file_path_(socket_file_path),
        options_(options),
        verify_peer_(verify_peer),
        socket_path_health_check_timer_(*this),
        work_guard_(asio::make_work_guard(io_ctx_)) {
    io_ctx_thread_ = std::thread([this] {
      io_ctx_.run();
    });
  }

  ~server() override {
    detach_from_dispatcher([this] {
      stop();
    });

    asio::post(
        io_ctx_,
        [this] {
          close_acceptor();
          close_all_peers();
          close_socket_path_health_check_peer();
          work_guard_.reset();
        });

    if (io_ctx_thread_.joinable()) {
      io_ctx_thread_.join();
    }
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
    asio::post(
        io_ctx_,
        [this, id, data] {
          if (auto it = peers_.find(id);
              it != peers_.end()) {
            it->second->async_send(data);
          }
        });
  }

  void async_respond(peer_id id,
                     request_id request_id_value,
                     const std::vector<uint8_t>& data) {
    asio::post(
        io_ctx_,
        [this, id, request_id_value, data] {
          if (auto it = peers_.find(id);
              it != peers_.end()) {
            it->second->async_send_response(request_id_value, data);
          }
        });
  }

  void async_close_peer(peer_id id) {
    asio::post(
        io_ctx_,
        [this, id] {
          close_peer(id);
        });
  }

private:
  // This method is executed in the dispatcher thread.
  void stop() {
    stopped_ = true;
    ++bind_retry_generation_;
    socket_path_health_check_timer_.stop();
    exposed_peer_ids_.clear();

    asio::post(
        io_ctx_,
        [this] {
          close_acceptor();
          close_all_peers();
          close_socket_path_health_check_peer();
          socket_path_health_check_in_progress_ = false;
        });
  }

  // This method is executed in the dispatcher thread.
  void bind() {
    asio::post(
        io_ctx_,
        [this] {
          if (stopped_ ||
              acceptor_) {
            return;
          }

          std::error_code remove_error_code;
          std::filesystem::remove(socket_file_path_, remove_error_code);

          acceptor_ = std::make_unique<asio::local::stream_protocol::acceptor>(io_ctx_);

          asio::error_code error_code;
          acceptor_->open(asio::local::stream_protocol::endpoint(socket_file_path_).protocol(), error_code);
          if (error_code) {
            handle_bind_failed(error_code);
            return;
          }

          acceptor_->bind(asio::local::stream_protocol::endpoint(socket_file_path_), error_code);
          if (error_code) {
            handle_bind_failed(error_code);
            return;
          }

          acceptor_->listen(asio::socket_base::max_listen_connections, error_code);
          if (error_code) {
            handle_bind_failed(error_code);
            return;
          }

          enqueue_to_dispatcher([this] {
            bound();
            start_socket_path_health_check_timer();
          });

          accept();
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void handle_bind_failed(const asio::error_code& error_code) {
    close_acceptor();

    enqueue_to_dispatcher([this, error_code] {
      bind_failed(error_code);
      schedule_bind_retry();
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void accept() {
    if (stopped_ ||
        !acceptor_) {
      return;
    }

    acceptor_->async_accept(
        [this](auto&& error_code, auto socket) {
          if (stopped_) {
            asio::error_code close_error_code;
            socket.close(close_error_code);
            return;
          }

          if (error_code) {
            if (error_code != asio::error::operation_aborted) {
              close_acceptor();

              enqueue_to_dispatcher([this] {
                closed();
                schedule_bind_retry();
              });
            }
            return;
          }

          handle_accepted_socket(std::move(socket));
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void handle_accepted_socket(asio::local::stream_protocol::socket socket) {
    if (stopped_) {
      asio::error_code close_error_code;
      socket.close(close_error_code);
      return;
    }

    auto credentials = impl::make_peer_credentials(socket);
    auto id = ++next_peer_id_;
    auto p = std::make_shared<impl::peer>(weak_dispatcher_,
                                          std::move(socket),
                                          options_);
    peers_[id] = p;

    p->ready.connect([this, id, credentials] {
      enqueue_to_dispatcher([this, id, credentials] {
        if (verify_peer_(credentials)) {
          exposed_peer_ids_.insert(id);
          peer_connected(id, credentials);
        } else {
          asio::post(
              io_ctx_,
              [this, id] {
                close_peer(id);
              });
        }
      });
    });

    p->received.connect([this, id](auto&& buffer) {
      enqueue_to_dispatcher([this, id, buffer] {
        if (exposed_peer_ids_.contains(id)) {
          received(id, buffer);
        }
      });
    });

    p->request_received.connect([this, id](auto request_id, auto&& buffer) {
      enqueue_to_dispatcher([this, id, request_id, buffer] {
        if (exposed_peer_ids_.contains(id)) {
          request_received(id, request_id, buffer);
        }
      });
    });

    p->error_occurred.connect([this, id](auto&& error_code) {
      enqueue_to_dispatcher([this, id, error_code] {
        if (exposed_peer_ids_.contains(id)) {
          peer_error_occurred(id, error_code);
        }
      });
    });

    p->closed.connect([this, id] {
      asio::post(
          io_ctx_,
          [this, id] {
            peers_.erase(id);
          });

      enqueue_to_dispatcher([this, id] {
        if (exposed_peer_ids_.erase(id) > 0) {
          peer_closed(id);
        }
      });
    });

    p->async_start();

    accept();
  }

  // This method is executed in `io_ctx_thread_`.
  void close_acceptor() {
    asio::error_code error_code;
    if (acceptor_) {
      acceptor_->cancel(error_code);
      acceptor_->close(error_code);
      acceptor_.reset();
    }

    std::error_code remove_error_code;
    std::filesystem::remove(socket_file_path_, remove_error_code);
  }

  // This method is executed in the dispatcher thread.
  void schedule_bind_retry() {
    socket_path_health_check_timer_.stop();
    ++bind_retry_generation_;

    if (stopped_) {
      return;
    }

    auto scheduled_bind_retry_generation = bind_retry_generation_;
    enqueue_to_dispatcher(
        [this, scheduled_bind_retry_generation] {
          if (stopped_ ||
              bind_retry_generation_ != scheduled_bind_retry_generation) {
            return;
          }

          bind();
        },
        when_now() + impl::normalize_scheduling_interval(options_.bind_retry_interval));
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
    asio::post(
        io_ctx_,
        [this] {
          if (stopped_ ||
              !acceptor_ ||
              socket_path_health_check_in_progress_) {
            return;
          }

          socket_path_health_check_in_progress_ = true;

          auto socket = std::make_shared<asio::local::stream_protocol::socket>(io_ctx_);
          auto timeout = std::make_shared<asio::steady_timer>(io_ctx_);

          timeout->expires_after(options_.socket_path_health_check_timeout);
          timeout->async_wait([this, socket](const auto& error_code) {
            if (!error_code) {
              asio::error_code close_error_code;
              socket->close(close_error_code);
              handle_socket_path_health_check_failed(asio::error::timed_out);
            }
          });

          socket->async_connect(
              asio::local::stream_protocol::endpoint(socket_file_path_),
              [this, socket, timeout](auto&& error_code) mutable {
                if (error_code) {
                  timeout->cancel();
                  handle_socket_path_health_check_failed(error_code);
                  return;
                }

                socket_path_health_check_peer_ = std::make_shared<impl::peer>(weak_dispatcher_,
                                                                              std::move(*socket),
                                                                              options_);

                socket_path_health_check_peer_->health_check_response_received.connect([this, timeout] {
                  asio::post(
                      io_ctx_,
                      [this, timeout] {
                        timeout->cancel();

                        close_socket_path_health_check_peer();

                        socket_path_health_check_in_progress_ = false;
                      });
                });

                socket_path_health_check_peer_->error_occurred.connect([this, timeout](auto&& error_code) {
                  asio::post(
                      io_ctx_,
                      [this, timeout, error_code] {
                        timeout->cancel();
                        handle_socket_path_health_check_failed(error_code);
                      });
                });

                socket_path_health_check_peer_->closed.connect([this, timeout] {
                  asio::post(
                      io_ctx_,
                      [this, timeout] {
                        timeout->cancel();

                        if (socket_path_health_check_in_progress_) {
                          handle_socket_path_health_check_failed(asio::error::eof);
                        }
                      });
                });

                socket_path_health_check_peer_->async_start();
                socket_path_health_check_peer_->async_send_health_check();
              });
        });
  }

  // This method is executed in `io_ctx_thread_`.
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

  // This method is executed in `io_ctx_thread_`.
  void close_peer(peer_id id) {
    if (auto it = peers_.find(id);
        it != peers_.end()) {
      it->second->async_close();
      peers_.erase(it);
    }
  }

  // This method is executed in `io_ctx_thread_`.
  void close_socket_path_health_check_peer() {
    if (socket_path_health_check_peer_) {
      socket_path_health_check_peer_->async_close();
      socket_path_health_check_peer_.reset();
    }
  }

  // This method is executed in `io_ctx_thread_`.
  void close_all_peers() {
    for (auto& [_, p] : peers_) {
      if (p) {
        p->async_close();
      }
    }

    peers_.clear();
  }

  std::filesystem::path socket_file_path_;
  server_options options_;
  std::function<bool(const peer_credentials&)> verify_peer_;
  dispatcher::extra::timer socket_path_health_check_timer_;
  std::atomic_bool stopped_ = true;
  int bind_retry_generation_ = 0;

  asio::io_context io_ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread io_ctx_thread_;
  std::unique_ptr<asio::local::stream_protocol::acceptor> acceptor_;
  std::unordered_map<peer_id, std::shared_ptr<impl::peer>> peers_;
  std::unordered_set<peer_id> exposed_peer_ids_;
  std::shared_ptr<impl::peer> socket_path_health_check_peer_;
  bool socket_path_health_check_in_progress_ = false;
  peer_id next_peer_id_ = 0;
};

} // namespace pqrs::unix_domain_stream
