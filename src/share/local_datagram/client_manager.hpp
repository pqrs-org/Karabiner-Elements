#pragma once

// `krbn::local_datagram::client_manager` can be used safely in a multi-threaded environment.

#include "local_datagram/client.hpp"
#include "thread_utility.hpp"

namespace krbn {
namespace local_datagram {
class client_manager final {
public:
  // Signals

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  client_manager(const std::string& path,
                 boost::optional<std::chrono::milliseconds> server_check_interval,
                 std::chrono::milliseconds reconnect_interval) : path_(path),
                                                                 server_check_interval_(server_check_interval),
                                                                 reconnect_interval_(reconnect_interval),
                                                                 reconnect_timer_enabled_(true) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();
  }

  ~client_manager(void) {
    async_stop();

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  std::shared_ptr<client> get_client(void) {
    std::lock_guard<std::mutex> lock(client_mutex_);

    return client_;
  }

  void async_start(void) {
    dispatcher_->enqueue([this] {
      reconnect_timer_enabled_ = true;

      connect();
    });
  }

  void async_stop(void) {
    dispatcher_->enqueue([this] {
      // We have to unset reconnect_timer_enabled_ before `close` to prevent `start_reconnect_timer` by `closed` signal.
      reconnect_timer_enabled_ = false;

      stop_reconnect_timer();
      close();
    });
  }

private:
  void connect(void) {
    if (client_) {
      return;
    }

    // Guard client_ for `get_client`.

    {
      std::lock_guard<std::mutex> lock(client_mutex_);
      client_ = std::make_shared<client>();

      client_->connected.connect([this] {
        dispatcher_->enqueue([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        dispatcher_->enqueue([this, error_code] {
          connect_failed(error_code);

          close();
          stop_reconnect_timer();
          start_reconnect_timer();
        });
      });

      client_->closed.connect([this] {
        dispatcher_->enqueue([this] {
          closed();

          close();
          stop_reconnect_timer();
          start_reconnect_timer();
        });
      });

      client_->async_connect(path_,
                             server_check_interval_);
    }
  }

  void close(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;
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
          dispatcher_->enqueue([this] {
            connect();
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
  boost::optional<std::chrono::milliseconds> server_check_interval_;
  std::chrono::milliseconds reconnect_interval_;

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  std::shared_ptr<client> client_;
  std::mutex client_mutex_;

  std::unique_ptr<thread_utility::timer> reconnect_timer_;
  bool reconnect_timer_enabled_;
};
} // namespace local_datagram
} // namespace krbn
