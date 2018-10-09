#pragma once

// `krbn::local_datagram::server_manager` can be used safely in a multi-threaded environment.

#include "dispatcher.hpp"
#include "local_datagram/server.hpp"

namespace krbn {
namespace local_datagram {
class server_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(void)> bound;
  boost::signals2::signal<void(const boost::system::error_code&)> bind_failed;
  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(const std::shared_ptr<std::vector<uint8_t>>)> received;

  // Methods

  server_manager(const std::string& path,
                 size_t buffer_size,
                 boost::optional<std::chrono::milliseconds> server_check_interval,
                 std::chrono::milliseconds reconnect_interval) : dispatcher_client(),
                                                                 path_(path),
                                                                 buffer_size_(buffer_size),
                                                                 server_check_interval_(server_check_interval),
                                                                 reconnect_interval_(reconnect_interval),
                                                                 reconnect_enabled_(false) {
  }

  virtual ~server_manager(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      reconnect_enabled_ = true;

      bind();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void stop(void) {
    // We have to unset reconnect_enabled_ before `close` to prevent `enqueue_reconnect` by `closed` signal.
    reconnect_enabled_ = false;

    close();
  }

  // This method is executed in the dispatcher thread.
  void bind(void) {
    if (server_) {
      return;
    }

    server_ = std::make_unique<server>();

    server_->bound.connect([this] {
      enqueue_to_dispatcher([this] {
        bound();
      });
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      enqueue_to_dispatcher([this, error_code] {
        bind_failed(error_code);
      });

      close();
      enqueue_reconnect();
    });

    server_->closed.connect([this] {
      enqueue_to_dispatcher([this] {
        closed();
      });

      close();
      enqueue_reconnect();
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

  // This method is executed in the dispatcher thread.
  void close(void) {
    if (!server_) {
      return;
    }

    server_ = nullptr;
  }

  // This method is executed in the dispatcher thread.
  void enqueue_reconnect(void) {
    enqueue_to_dispatcher(
        [this] {
          if (!reconnect_enabled_) {
            return;
          }

          bind();
        },
        when_now() + reconnect_interval_);
  }

  std::string path_;
  size_t buffer_size_;
  boost::optional<std::chrono::milliseconds> server_check_interval_;
  std::chrono::milliseconds reconnect_interval_;

  std::unique_ptr<server> server_;
  bool reconnect_enabled_;
};
} // namespace local_datagram
} // namespace krbn
