#pragma once

// `krbn::local_datagram::server_manager` can be used safely in a multi-threaded environment.

#include "dispatcher.hpp"
#include "local_datagram/server.hpp"
#include "thread_utility.hpp"

namespace krbn {
namespace local_datagram {
class server_manager final : public pqrs::dispatcher::dispatcher_client {
public:
  // Signals

  boost::signals2::signal<void(void)> bound;
  boost::signals2::signal<void(const boost::system::error_code&)> bind_failed;
  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(const std::shared_ptr<std::vector<uint8_t>>)> received;

  // Methods

  server_manager(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                 const std::string& path,
                 size_t buffer_size,
                 boost::optional<std::chrono::milliseconds> server_check_interval,
                 std::chrono::milliseconds reconnect_interval) : dispatcher_client(weak_dispatcher),
                                                                 path_(path),
                                                                 buffer_size_(buffer_size),
                                                                 server_check_interval_(server_check_interval),
                                                                 reconnect_interval_(reconnect_interval),
                                                                 reconnect_timer_enabled_(false) {
  }

  virtual ~server_manager(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      reconnect_timer_enabled_ = true;

      bind();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    // We have to unset reconnect_timer_enabled_ before `close` to prevent `start_reconnect_timer` by `closed` signal.
    reconnect_timer_enabled_ = false;

    stop_reconnect_timer();
    close();
  }

  void bind(void) {
    if (server_) {
      return;
    }

    server_ = std::make_unique<server>(weak_dispatcher_);

    server_->bound.connect([this] {
      enqueue_to_dispatcher([this] {
        bound();
      });
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        bind_failed(error_code);

        close();
        stop_reconnect_timer();
        start_reconnect_timer();
      });
    });

    server_->closed.connect([this] {
      enqueue_to_dispatcher([this] {
        closed();

        close();
        stop_reconnect_timer();
        start_reconnect_timer();
      });
    });

    server_->received.connect([this](auto&& buffer) {
      enqueue_to_dispatcher([this, buffer] {
        received(buffer);
      });
    });

    server_->async_bind(path_,
                        buffer_size_,
                        server_check_interval_);
  }

  void close(void) {
    if (!server_) {
      return;
    }

    server_ = nullptr;
  }

  void start_reconnect_timer(void) {
    if (reconnect_timer_) {
      return;
    }

    if (!reconnect_timer_enabled_) {
      return;
    }

    reconnect_timer_ = std::make_unique<thread_utility::timer>(
        reconnect_interval_,
        thread_utility::timer::mode::once,
        [this] {
          enqueue_to_dispatcher([this] {
            bind();
          });
        });
  }

  void stop_reconnect_timer(void) {
    if (!reconnect_timer_) {
      return;
    }

    reconnect_timer_->cancel();
    reconnect_timer_ = nullptr;
  }

  std::string path_;
  size_t buffer_size_;
  boost::optional<std::chrono::milliseconds> server_check_interval_;
  std::chrono::milliseconds reconnect_interval_;

  std::unique_ptr<server> server_;
  std::unique_ptr<thread_utility::timer> reconnect_timer_;
  bool reconnect_timer_enabled_;
};
} // namespace local_datagram
} // namespace krbn
