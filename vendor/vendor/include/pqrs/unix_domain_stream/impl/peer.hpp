#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../options.hpp"
#include "asio_helper.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <atomic>
#include <deque>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace pqrs::unix_domain_stream::impl {

class peer final : public dispatcher::extra::dispatcher_client,
                   public std::enable_shared_from_this<peer> {
public:
  nod::signal<void()> ready;
  nod::signal<void(not_null_shared_ptr_t<std::vector<uint8_t>>)> received;
  nod::signal<void(uint64_t, not_null_shared_ptr_t<std::vector<uint8_t>>)> request_received;
  nod::signal<void(uint64_t, not_null_shared_ptr_t<std::vector<uint8_t>>)> response_received;
  nod::signal<void()> health_check_response_received;
  nod::signal<void(const asio::error_code&)> error_occurred;
  nod::signal<void()> closed;

  peer(const peer&) = delete;

  peer(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
       asio::local::stream_protocol::socket socket,
       const common_options& options)
      : dispatcher_client(weak_dispatcher),
        socket_(std::move(socket)),
        options_(options),
        ready_deadline_(socket_.get_executor()),
        heartbeat_timer_(socket_.get_executor()),
        heartbeat_deadline_(socket_.get_executor()),
        read_deadline_(socket_.get_executor()),
        write_deadline_(socket_.get_executor()) {
  }

  // The owner must call async_close before releasing the last shared_ptr so
  // socket and timer state is closed on `socket_.get_executor()`.
  ~peer() override {
    if (!closed_on_executor_.load()) {
      abort();
    }

    detach_from_dispatcher();
  }

  void async_start() {
    asio::post(
        socket_.get_executor(),
        [self = shared_from_this()] {
          self->start_ready_deadline();
          self->start_heartbeat_timer();
          self->refresh_heartbeat_deadline();
          self->read_header();
        });
  }

  void async_close() {
    asio::post(
        socket_.get_executor(),
        [self = shared_from_this()] {
          self->close();
        });
  }

  void async_send(const std::vector<uint8_t>& data) {
    auto frame = protocol::make_user_data_frame(data);

    asio::post(
        socket_.get_executor(),
        [self = shared_from_this(), frame = std::move(frame)] {
          self->push_frame(frame);
        });
  }

  void async_send_request(uint64_t request_id,
                          const std::vector<uint8_t>& data) {
    auto frame = protocol::make_request_frame(request_id, data);

    asio::post(
        socket_.get_executor(),
        [self = shared_from_this(), frame = std::move(frame)] {
          self->push_frame(frame);
        });
  }

  void async_send_response(uint64_t request_id,
                           const std::vector<uint8_t>& data) {
    auto frame = protocol::make_response_frame(request_id, data);

    asio::post(
        socket_.get_executor(),
        [self = shared_from_this(), frame = std::move(frame)] {
          self->push_frame(frame);
        });
  }

  void async_send_health_check() {
    auto frame = protocol::make_health_check_frame();

    asio::post(
        socket_.get_executor(),
        [self = shared_from_this(), frame = std::move(frame)] {
          self->push_frame(frame);
        });
  }

private:
  // This method is executed in `io_ctx_thread_`.
  void start_heartbeat_timer() {
    heartbeat_timer_.expires_after(normalize_scheduling_interval(options_.heartbeat_interval));

    heartbeat_timer_.async_wait([self = shared_from_this()](const auto& error_code) {
      if (!error_code &&
          self->socket_.is_open()) {
        self->push_frame(protocol::make_heartbeat_frame());
        self->start_heartbeat_timer();
      }
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void refresh_heartbeat_deadline() {
    heartbeat_deadline_.expires_after(options_.heartbeat_timeout);

    heartbeat_deadline_.async_wait([self = shared_from_this()](const auto& error_code) {
      if (!error_code) {
        self->handle_error(asio::error::timed_out);
      }
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void start_ready_deadline() {
    ready_deadline_.expires_after(std::chrono::milliseconds(100));

    ready_deadline_.async_wait([self = shared_from_this()](const auto& error_code) {
      if (!error_code) {
        self->ensure_ready();
      }
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void ensure_ready() {
    if (ready_) {
      return;
    }

    ready_ = true;
    ready_deadline_.cancel();

    enqueue_to_dispatcher([this] {
      ready();
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void read_header() {
    if (!socket_.is_open()) {
      return;
    }

    start_read_deadline();

    asio::async_read(
        socket_,
        asio::buffer(read_header_),
        [self = shared_from_this()](auto&& error_code, auto bytes_transferred) {
          self->read_deadline_.cancel();

          if (error_code) {
            self->handle_error(error_code);
            return;
          }

          if (bytes_transferred != protocol::header_size) {
            self->handle_error(asio::error::message_size);
            return;
          }

          auto body_size = protocol::decode_uint32(self->read_header_);
          if (body_size < protocol::type_size ||
              body_size > self->options_.max_message_size + protocol::type_size + protocol::request_id_size) {
            self->handle_error(asio::error::message_size);
            return;
          }

          self->read_body_.resize(body_size);
          self->read_body();
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void read_body() {
    start_read_deadline();

    asio::async_read(
        socket_,
        asio::buffer(read_body_),
        [self = shared_from_this()](auto&& error_code, auto bytes_transferred) {
          self->read_deadline_.cancel();

          if (error_code) {
            self->handle_error(error_code);
            return;
          }

          if (bytes_transferred != self->read_body_.size()) {
            self->handle_error(asio::error::message_size);
            return;
          }

          self->refresh_heartbeat_deadline();

          auto type = static_cast<protocol::message_type>(self->read_body_[0]);
          switch (type) {
            case protocol::message_type::heartbeat:
              break;

            case protocol::message_type::user_data: {
              self->ensure_ready();

              if (self->read_body_.size() > self->options_.max_message_size + protocol::type_size) {
                self->handle_error(asio::error::message_size);
                return;
              }

              auto v = std::make_shared<std::vector<uint8_t>>(std::begin(self->read_body_) + protocol::type_size,
                                                              std::end(self->read_body_));
              self->enqueue_to_dispatcher([p = self.get(), v] {
                p->received(v);
              });
              break;
            }

            case protocol::message_type::request:
            case protocol::message_type::response: {
              self->ensure_ready();

              if (self->read_body_.size() < protocol::type_size + protocol::request_id_size ||
                  self->read_body_.size() > self->options_.max_message_size + protocol::type_size + protocol::request_id_size) {
                self->handle_error(asio::error::message_size);
                return;
              }

              auto request_id = protocol::decode_uint64(self->read_body_,
                                                        protocol::type_size);
              auto v = std::make_shared<std::vector<uint8_t>>(std::begin(self->read_body_) + protocol::type_size + protocol::request_id_size,
                                                              std::end(self->read_body_));

              if (type == protocol::message_type::request) {
                self->enqueue_to_dispatcher([p = self.get(), request_id, v] {
                  p->request_received(request_id, v);
                });
              } else {
                self->enqueue_to_dispatcher([p = self.get(), request_id, v] {
                  p->response_received(request_id, v);
                });
              }
              break;
            }

            case protocol::message_type::health_check:
              if (self->ready_) {
                self->handle_error(asio::error::operation_not_supported);
              } else {
                self->close_after_write_ = true;
                self->push_frame(protocol::make_health_check_response_frame());
              }
              return;

            case protocol::message_type::health_check_response:
              self->enqueue_to_dispatcher([p = self.get()] {
                p->health_check_response_received();
              });
              break;

            default:
              self->handle_error(asio::error::invalid_argument);
              return;
          }

          self->read_header();
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void start_read_deadline() {
    read_deadline_.expires_after(options_.read_timeout);

    read_deadline_.async_wait([self = shared_from_this()](const auto& error_code) {
      if (!error_code) {
        self->handle_error(asio::error::timed_out);
      }
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void push_frame(std::vector<uint8_t> frame) {
    if (!socket_.is_open()) {
      return;
    }

    if (!valid_outgoing_frame(frame) ||
        write_queue_.size() >= options_.max_send_queue_size) {
      handle_error(asio::error::no_buffer_space);
      return;
    }

    auto was_empty = write_queue_.empty();
    write_queue_.push_back(std::move(frame));

    if (was_empty) {
      write();
    }
  }

  // This method is executed in `io_ctx_thread_`.
  bool valid_outgoing_frame(const std::vector<uint8_t>& frame) const {
    if (frame.size() < protocol::header_size + protocol::type_size) {
      return false;
    }

    std::array<uint8_t, protocol::header_size> header;
    std::copy_n(std::begin(frame), protocol::header_size, std::begin(header));

    auto body_size = protocol::decode_uint32(header);
    if (frame.size() != protocol::header_size + body_size ||
        body_size < protocol::type_size) {
      return false;
    }

    auto type = static_cast<protocol::message_type>(frame[protocol::header_size]);
    switch (type) {
      case protocol::message_type::request:
      case protocol::message_type::response:
        return body_size >= protocol::type_size + protocol::request_id_size &&
               body_size <= options_.max_message_size + protocol::type_size + protocol::request_id_size;

      case protocol::message_type::heartbeat:
      case protocol::message_type::user_data:
      case protocol::message_type::health_check:
      case protocol::message_type::health_check_response:
        return body_size <= options_.max_message_size + protocol::type_size;
    }

    return false;
  }

  // This method is executed in `io_ctx_thread_`.
  void write() {
    if (!socket_.is_open() ||
        write_queue_.empty()) {
      return;
    }

    write_deadline_.expires_after(options_.write_timeout);

    write_deadline_.async_wait([self = shared_from_this()](const auto& error_code) {
      if (!error_code) {
        self->handle_error(asio::error::timed_out);
      }
    });

    asio::async_write(
        socket_,
        asio::buffer(write_queue_.front()),
        [self = shared_from_this()](auto&& error_code, auto) {
          self->write_deadline_.cancel();

          if (error_code) {
            self->handle_error(error_code);
            return;
          }

          self->write_queue_.pop_front();

          if (self->write_queue_.empty() &&
              self->close_after_write_) {
            self->close();
            return;
          }

          self->write();
        });
  }

  // This method is executed in `io_ctx_thread_`.
  void handle_error(const asio::error_code& error_code) {
    if (error_code == asio::error::operation_aborted) {
      return;
    }

    enqueue_to_dispatcher([this, error_code] {
      error_occurred(error_code);
    });

    close();
  }

  // This method is executed in `io_ctx_thread_`.
  void close() {
    if (!socket_.is_open()) {
      return;
    }

    close_socket();

    enqueue_to_dispatcher([this] {
      closed();
    });
  }

  // This method is executed in `io_ctx_thread_`.
  void close_socket() {
    closed_on_executor_ = true;

    asio::error_code error_code;
    ready_deadline_.cancel();
    heartbeat_timer_.cancel();
    heartbeat_deadline_.cancel();
    read_deadline_.cancel();
    write_deadline_.cancel();
    socket_.cancel(error_code);
    socket_.close(error_code);
  }

  asio::local::stream_protocol::socket socket_;
  common_options options_;
  std::atomic_bool closed_on_executor_ = false;
  bool ready_ = false;
  bool close_after_write_ = false;
  asio::steady_timer ready_deadline_;
  asio::steady_timer heartbeat_timer_;
  asio::steady_timer heartbeat_deadline_;
  asio::steady_timer read_deadline_;
  asio::steady_timer write_deadline_;
  std::array<uint8_t, protocol::header_size> read_header_;
  std::vector<uint8_t> read_body_;
  std::deque<std::vector<uint8_t>> write_queue_;
};

} // namespace pqrs::unix_domain_stream::impl
