#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::client` can be used safely in a multi-threaded environment.

#include "impl/client.hpp"
#include "request_id.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
class client final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(const asio::error_code&)> error_occurred;
  nod::signal<void(request_id)> processed;

  // Methods

  client(const client&) = delete;

  client(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
         const std::string& path,
         size_t buffer_size) : dispatcher_client(weak_dispatcher),
                               path_(path),
                               buffer_size_(buffer_size),
                               last_request_id_(0),
                               reconnect_timer_(*this) {
    impl_client_ = std::make_shared<impl::client>(weak_dispatcher_);

    impl_client_->connected.connect([this] {
      enqueue_to_dispatcher([this] {
        connected();
      });
    });

    impl_client_->connect_failed.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        connect_failed(error_code);
      });

      if (impl_client_) {
        impl_client_->async_close();
      }

      start_reconnect_timer();
    });

    impl_client_->closed.connect([this] {
      enqueue_to_dispatcher([this] {
        closed();
      });

      start_reconnect_timer();
    });

    impl_client_->error_occurred.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        error_occurred(error_code);
      });
    });

    impl_client_->processed.connect([this](auto&& request_id) {
      enqueue_to_dispatcher([this, request_id] {
        processed(request_id);
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

  // You have to call `set_reconnect_interval` before `async_start`.
  void set_reconnect_interval(std::optional<std::chrono::milliseconds> value) {
    reconnect_interval_ = value;
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

  request_id async_send(const std::vector<uint8_t>& v) {
    auto request_id = make_request_id();
    auto ptr = std::make_shared<impl::buffer>(impl::buffer::type::user_data,
                                              request_id,
                                              v);

    enqueue_to_dispatcher([this, request_id, ptr] {
      if (impl_client_) {
        impl_client_->async_send(ptr);
      } else {
        processed(request_id);
      }
    });

    return request_id;
  }

  request_id async_send(const uint8_t* p, size_t length) {
    auto request_id = make_request_id();
    auto ptr = std::make_shared<impl::buffer>(impl::buffer::type::user_data,
                                              request_id,
                                              p,
                                              length);

    enqueue_to_dispatcher([this, request_id, ptr] {
      if (impl_client_) {
        impl_client_->async_send(ptr);
      } else {
        processed(request_id);
      }
    });

    return request_id;
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
    if (impl_client_) {
      impl_client_->async_connect(path_,
                                  buffer_size_,
                                  server_check_interval_);
    }
  }

  // This method is executed in the dispatcher thread.
  void close(void) {
    impl_client_ = nullptr;
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

  request_id make_request_id(void) {
    std::lock_guard<std::mutex> lock(last_request_id_mutex_);

    return ++last_request_id_;
  }

  std::string path_;
  size_t buffer_size_;
  request_id last_request_id_;
  std::mutex last_request_id_mutex_;
  std::optional<std::chrono::milliseconds> server_check_interval_;
  std::optional<std::chrono::milliseconds> reconnect_interval_;
  std::shared_ptr<impl::client> impl_client_;
  dispatcher::extra::timer reconnect_timer_;
};
} // namespace local_datagram
} // namespace pqrs
