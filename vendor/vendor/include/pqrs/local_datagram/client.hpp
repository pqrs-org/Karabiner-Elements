#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::client` can be used safely in a multi-threaded environment.

#include "impl/client_impl.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <unordered_map>

namespace pqrs {
namespace local_datagram {
class client final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const std::string&)> warning_reported;
  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(const asio::error_code&)> error_occurred;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>, std::shared_ptr<asio::local::datagram_protocol::endpoint>)> received;
  nod::signal<void(std::shared_ptr<asio::local::datagram_protocol::endpoint> sender_endpoint)> next_heartbeat_deadline_exceeded;

  // Methods

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::filesystem::path& server_socket_file_path,
         const std::optional<std::filesystem::path>& client_socket_file_path,
         size_t buffer_size) : dispatcher_client(weak_dispatcher),
                               server_socket_file_path_(server_socket_file_path),
                               client_socket_file_path_(client_socket_file_path),
                               buffer_size_(buffer_size),
                               server_socket_file_path_resolver_(nullptr),
                               client_send_entries_(std::make_shared<std::deque<std::shared_ptr<impl::send_entry>>>()),
                               reconnect_timer_(*this) {
    client_impl_ = std::make_shared<impl::client_impl>(
        weak_dispatcher_,
        client_send_entries_);

    client_impl_->warning_reported.connect([this](auto&& message) {
      enqueue_to_dispatcher([this, message] {
        warning_reported(message);
      });
    });

    client_impl_->connected.connect([this] {
      enqueue_to_dispatcher([this] {
        connected();
      });
    });

    auto connect_failed_handler = [this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        connect_failed(error_code);
      });

      if (client_impl_) {
        client_impl_->async_close();
      }

      start_reconnect_timer();
    };

    client_impl_->connect_failed.connect([connect_failed_handler](auto&& error_code) {
      connect_failed_handler(error_code);
    });

    client_impl_->bind_failed.connect([connect_failed_handler](auto&& error_code) {
      connect_failed_handler(error_code);
    });

    client_impl_->closed.connect([this] {
      enqueue_to_dispatcher([this] {
        closed();
      });

      start_reconnect_timer();
    });

    client_impl_->error_occurred.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        error_occurred(error_code);
      });
    });

    client_impl_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
      enqueue_to_dispatcher([this, buffer, sender_endpoint] {
        received(buffer, sender_endpoint);
      });
    });

    client_impl_->next_heartbeat_deadline_exceeded.connect([this](auto&& sender_endpoint) {
      enqueue_to_dispatcher([this, sender_endpoint] {
        next_heartbeat_deadline_exceeded(sender_endpoint);
      });
    });
  }

  virtual ~client(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  // You have to call `set_server_check_interval` before `async_start`.
  void set_server_check_interval(std::optional<std::chrono::milliseconds> value) {
    server_check_interval_ = value;
  }

  // You have to call `set_next_heartbeat_deadline` before `async_start`.
  void set_next_heartbeat_deadline(std::optional<std::chrono::milliseconds> value) {
    next_heartbeat_deadline_ = value;
  }

  // You have to call `set_client_socket_check_interval` before `async_start`.
  void set_client_socket_check_interval(std::optional<std::chrono::milliseconds> value) {
    client_socket_check_interval_ = value;
  }

  // You have to call `set_reconnect_interval` before `async_start`.
  void set_reconnect_interval(std::optional<std::chrono::milliseconds> value) {
    reconnect_interval_ = value;
  }

  void set_server_socket_file_path_resolver(std::function<std::filesystem::path(void)> value) {
    server_socket_file_path_resolver_ = value;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      connect();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_send(const std::vector<uint8_t>& v,
                  std::function<void(void)> processed = nullptr) {
    auto entry = std::make_shared<impl::send_entry>(impl::send_entry::type::user_data,
                                                    v,
                                                    nullptr,
                                                    processed);
    async_send(entry);
  }

  void async_send(const uint8_t* p,
                  size_t length,
                  std::function<void(void)> processed = nullptr) {
    auto entry = std::make_shared<impl::send_entry>(impl::send_entry::type::user_data,
                                                    p,
                                                    length,
                                                    nullptr,
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
  void connect(void) {
    if (client_impl_) {
      std::filesystem::path server_socket_file_path;

      if (server_socket_file_path_resolver_) {
        server_socket_file_path = server_socket_file_path_resolver_();
      } else {
        server_socket_file_path = server_socket_file_path_;
      }

      client_impl_->async_connect(server_socket_file_path,
                                  client_socket_file_path_,
                                  buffer_size_,
                                  server_check_interval_,
                                  next_heartbeat_deadline_,
                                  client_socket_check_interval_);
    }
  }

  // This method is executed in the dispatcher thread.
  void close(void) {
    client_impl_ = nullptr;
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

                  connect();
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
      if (client_impl_) {
        client_impl_->async_send(entry);
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
  std::optional<std::filesystem::path> client_socket_file_path_;
  size_t buffer_size_;
  std::optional<std::chrono::milliseconds> server_check_interval_;
  std::optional<std::chrono::milliseconds> next_heartbeat_deadline_;
  std::optional<std::chrono::milliseconds> client_socket_check_interval_;
  std::optional<std::chrono::milliseconds> reconnect_interval_;
  std::function<std::filesystem::path(void)> server_socket_file_path_resolver_;

  std::shared_ptr<std::deque<std::shared_ptr<impl::send_entry>>> client_send_entries_;
  std::shared_ptr<impl::client_impl> client_impl_;
  dispatcher::extra::timer reconnect_timer_;
};
} // namespace local_datagram
} // namespace pqrs
