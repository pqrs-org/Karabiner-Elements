#pragma once

// `krbn::local_datagram::server_manager` can be used safely in a multi-threaded environment.

#include "local_datagram/server.hpp"
#include "thread_utility.hpp"

namespace krbn {
namespace local_datagram {
class server_manager final {
public:
  // Signals

  // Note: These signals are fired on server's thread.

  boost::signals2::signal<void(void)> bound;
  boost::signals2::signal<void(const boost::system::error_code&)> bind_failed;
  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(const boost::asio::mutable_buffer&)> received;

  // Methods

  server_manager(const std::string& path,
                 size_t buffer_size,
                 boost::optional<std::chrono::milliseconds> server_check_interval,
                 std::chrono::milliseconds reconnect_interval) : path_(path),
                                                                 buffer_size_(buffer_size),
                                                                 server_check_interval_(server_check_interval),
                                                                 reconnect_interval_(reconnect_interval),
                                                                 stopped_(true) {
  }

  ~server_manager(void) {
    stop();
  }

  std::shared_ptr<server> get_server(void) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    return server_;
  }

  void start(void) {
    stop();

    stopped_ = false;

    connect();
  }

  void stop(void) {
    // We have to set stopped_ before `close` to prevent `start_reconnect_thread` by `closed` signal.
    stopped_ = true;

    stop_reconnect_thread();
    close();
  }

private:
  void connect(void) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    server_ = nullptr;
    server_ = std::make_shared<server>();

    server_->bound.connect([this] {
      bound();
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      bind_failed(error_code);

      if (!stopped_) {
        start_reconnect_thread();
      }
    });

    server_->closed.connect([this] {
      closed();

      if (!stopped_) {
        start_reconnect_thread();
      }
    });

    server_->received.connect([this](auto&& buffer) {
      received(buffer);
    });

    server_->bind(path_,
                  buffer_size_,
                  server_check_interval_);
  }

  void close(void) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    server_ = nullptr;
  }

  void start_reconnect_thread(void) {
    std::lock_guard<std::mutex> lock(reconnect_timer_mutex_);

    reconnect_timer_ = std::make_unique<thread_utility::timer>(
        reconnect_interval_,
        false,
        [this] {
          connect();
        });
  }

  void stop_reconnect_thread(void) {
    std::lock_guard<std::mutex> lock(reconnect_timer_mutex_);

    reconnect_timer_ = nullptr;
  }

  std::string path_;
  size_t buffer_size_;
  boost::optional<std::chrono::milliseconds> server_check_interval_;
  std::chrono::milliseconds reconnect_interval_;

  bool stopped_;

  std::shared_ptr<server> server_;
  std::mutex server_mutex_;

  std::unique_ptr<thread_utility::timer> reconnect_timer_;
  std::mutex reconnect_timer_mutex_;
};
} // namespace local_datagram
} // namespace krbn
