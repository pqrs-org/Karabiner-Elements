#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::server` can be used safely in a multi-threaded environment.

#include "impl/server_impl.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
class server final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const std::string&)> warning_reported;
  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>, std::shared_ptr<asio::local::datagram_protocol::endpoint>)> received;
  nod::signal<void(std::shared_ptr<asio::local::datagram_protocol::endpoint> sender_endpoint)> next_heartbeat_deadline_exceeded;

  // Methods

  server(const server&) = delete;

  server(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& server_socket_file_path,
         size_t buffer_size) : dispatcher_client(weak_dispatcher),
                               server_socket_file_path_(server_socket_file_path),
                               buffer_size_(buffer_size),
                               server_send_entries_(std::make_shared<std::deque<std::shared_ptr<impl::send_entry>>>()),
                               reconnect_timer_(*this) {
  }

  virtual ~server(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  // You have to call `set_server_check_interval` before `async_start`.
  void set_server_check_interval(std::optional<std::chrono::milliseconds> value) {
    server_check_interval_ = value;
  }

  // You have to call `set_reconnect_interval` before `async_start`.
  void set_reconnect_interval(std::optional<std::chrono::milliseconds> value) {
    reconnect_interval_ = value;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      bind();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_send(const std::vector<uint8_t>& v,
                  std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint,
                  std::function<void(void)> processed = nullptr) {
    auto entry = std::make_shared<impl::send_entry>(impl::send_entry::type::user_data,
                                                    v,
                                                    destination_endpoint,
                                                    processed);
    async_send(entry);
  }

  void async_send(const uint8_t* p,
                  size_t length,
                  std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint,
                  std::function<void(void)> processed = nullptr) {
    auto entry = std::make_shared<impl::send_entry>(impl::send_entry::type::user_data,
                                                    p,
                                                    length,
                                                    destination_endpoint,
                                                    processed);
    async_send(entry);
  }

private:
  // This method is executed in the dispatcher thread.
  void stop(void) {
    // We have to unset reconnect_interval_ before `close` to prevent `start_reconnect_timer` by `closed` signal.
    reconnect_interval_ = std::nullopt;

    close();
  }

  // This method is executed in the dispatcher thread.
  void bind(void) {
    if (server_impl_) {
      return;
    }

    server_impl_ = std::make_unique<impl::server_impl>(weak_dispatcher_,
                                                       server_send_entries_);

    server_impl_->warning_reported.connect([this](auto&& message) {
      enqueue_to_dispatcher([this, message] {
        warning_reported(message);
      });
    });

    server_impl_->bound.connect([this] {
      enqueue_to_dispatcher([this] {
        bound();
      });
    });

    server_impl_->bind_failed.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        bind_failed(error_code);
      });

      close();
      start_reconnect_timer();
    });

    server_impl_->closed.connect([this] {
      enqueue_to_dispatcher([this] {
        closed();
      });

      close();
      start_reconnect_timer();
    });

    server_impl_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
      enqueue_to_dispatcher([this, buffer, sender_endpoint] {
        received(buffer, sender_endpoint);
      });
    });

    server_impl_->next_heartbeat_deadline_exceeded.connect([this](auto&& sender_endpoint) {
      enqueue_to_dispatcher([this, sender_endpoint] {
        next_heartbeat_deadline_exceeded(sender_endpoint);
      });
    });

    server_impl_->async_bind(server_socket_file_path_,
                             buffer_size_,
                             server_check_interval_);
  }

  // This method is executed in the dispatcher thread.
  void close(void) {
    if (!server_impl_) {
      return;
    }

    server_impl_ = nullptr;
  }

  // This method is executed in the dispatcher thread.
  void start_reconnect_timer(void) {
    if (reconnect_interval_) {
      enqueue_to_dispatcher(
          [this] {
            reconnect_timer_.start(
                [this] {
                  if (!reconnect_interval_) {
                    reconnect_timer_.stop();
                  }

                  bind();
                },
                *reconnect_interval_);
          },
          when_now() + *reconnect_interval_);
    } else {
      reconnect_timer_.stop();
    }
  }

  void async_send(std::shared_ptr<impl::send_entry> entry) {
    enqueue_to_dispatcher([this, entry] {
      if (server_impl_) {
        server_impl_->async_send(entry);
      } else {
        //
        // Call `processed`
        //

        auto&& processed = entry->get_processed();
        if (processed) {
          enqueue_to_dispatcher([processed] {
            processed();
          });
        }
      }
    });
  }

  std::filesystem::path server_socket_file_path_;
  size_t buffer_size_;
  std::optional<std::chrono::milliseconds> server_check_interval_;
  std::optional<std::chrono::milliseconds> reconnect_interval_;
  std::shared_ptr<std::deque<std::shared_ptr<impl::send_entry>>> server_send_entries_;
  std::unique_ptr<impl::server_impl> server_impl_;
  dispatcher::extra::timer reconnect_timer_;
};
} // namespace local_datagram
} // namespace pqrs
