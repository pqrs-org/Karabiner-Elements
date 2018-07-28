#pragma once

#include "local_datagram/client.hpp"
#include "thread_utility.hpp"

namespace krbn {
namespace local_datagram {
class client_manager final {
public:
  // Signals

  // Note: These signals are fired on client's thread.

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  client_manager(const std::string& path,
                 boost::optional<std::chrono::milliseconds> heartbeat_interval,
                 std::chrono::milliseconds reconnect_interval) : path_(path),
                                                                 heartbeat_interval_(heartbeat_interval),
                                                                 reconnect_interval_(reconnect_interval) {
  }

  ~client_manager(void) {
    stop();
  }

  void start(void) {
    stop();
    connect();
  }

  void stop(void) {
    stop_reconnect_thread();
    close();
  }

private:
  void connect(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    client_ = nullptr;
    client_ = std::make_unique<client>();

    client_->connected.connect([this](void) {
      connected();
    });

    client_->connect_failed.connect([this](auto&& error_code) {
      connect_failed(error_code);

      start_reconnect_thread();
    });

    client_->closed.connect([this](void) {
      closed();

      start_reconnect_thread();
    });

    client_->connect(path_,
                     heartbeat_interval_);
  }

  void close(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    client_ = nullptr;
  }

  void start_reconnect_thread(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    reconnect_timer_ = std::make_unique<thread_utility::timer>(reconnect_interval_, [this] {
      connect();
    });
  }

  void stop_reconnect_thread(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    reconnect_timer_ = nullptr;
  }

  std::string path_;
  boost::optional<std::chrono::milliseconds> heartbeat_interval_;
  std::chrono::milliseconds reconnect_interval_;

  std::unique_ptr<client> client_;
  std::unique_ptr<thread_utility::timer> reconnect_timer_;
  std::mutex mutex_;
};
} // namespace local_datagram
} // namespace krbn
