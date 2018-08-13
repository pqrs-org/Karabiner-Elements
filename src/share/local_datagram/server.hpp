#pragma once

// `krbn::local_datagram::server` can be used safely in a multi-threaded environment.

#include "local_datagram/client.hpp"
#include <unistd.h>

namespace krbn {
namespace local_datagram {
class server final {
public:
  // Signals

  // Note: These signals are fired on non-main thread.

  boost::signals2::signal<void(void)> bound;
  boost::signals2::signal<void(const boost::system::error_code&)> bind_failed;
  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(const boost::asio::mutable_buffer&)> received;

  // Methods

  server(const server&) = delete;

  server(void) : io_service_(),
                 work_(std::make_unique<boost::asio::io_service::work>(io_service_)),
                 socket_(io_service_),
                 bound_(false),
                 server_check_enabled_(false) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  ~server(void) {
    close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }
  }

  void bind(const std::string& path,
            size_t buffer_size,
            boost::optional<std::chrono::milliseconds> server_check_interval) {
    close();

    io_service_.post([this, path, buffer_size, server_check_interval] {
      bound_ = false;
      bound_path_.clear();

      // Open

      {
        boost::system::error_code error_code;
        socket_.open(boost::asio::local::datagram_protocol::socket::protocol_type(),
                     error_code);
        if (error_code) {
          bind_failed(error_code);
          return;
        }
      }

      // Bind

      {
        boost::system::error_code error_code;
        socket_.bind(boost::asio::local::datagram_protocol::endpoint(path),
                     error_code);

        if (error_code) {
          bind_failed(error_code);
          return;
        }
      }

      // Signal

      bound_ = true;
      bound_path_ = path;

      start_server_check_thread(path,
                                server_check_interval);

      bound();

      // Receive

      buffer_.resize(buffer_size);
      async_receive();
    });
  }

  void close(void) {
    io_service_.post([this] {
      stop_server_check_thread();

      // Close socket

      boost::system::error_code error_code;

      socket_.cancel(error_code);
      socket_.close(error_code);

      // Signal

      if (bound_) {
        bound_ = false;
        unlink(bound_path_.c_str());

        closed();
      }
    });
  }

private:
  void async_receive(void) {
    socket_.async_receive(boost::asio::buffer(buffer_),
                          [this](auto&& error_code, auto&& bytes_transferred) {
                            if (!error_code) {
                              received(boost::asio::buffer(buffer_, bytes_transferred));
                            }

                            // receive once if not closed

                            if (bound_) {
                              async_receive();
                            }
                          });
  }

  void start_server_check_thread(const std::string& path,
                                 boost::optional<std::chrono::milliseconds> server_check_interval) {
    stop_server_check_thread();

    if (server_check_interval) {
      server_check_enabled_ = true;

      server_check_thread_ = std::thread([this, path, server_check_interval] {
        std::unique_ptr<client> c;
        bool client_connected = false;

        while (server_check_enabled_) {
          server_check_timer_ = std::make_shared<thread_utility::timer>(
              *server_check_interval,
              false,
              [this, path, &c, &client_connected] {
                // Skip if client is connecting
                if (c && !client_connected) {
                  return;
                }

                c = std::make_unique<client>();

                c->connected.connect([&client_connected] {
                  client_connected = true;
                });

                c->connect_failed.connect([this](auto&& error_code) {
                  close();
                });

                c->connect(path, boost::none);
              });

          server_check_timer_->wait();
        }
      });
    }
  }

  void stop_server_check_thread(void) {
    server_check_enabled_ = false;

    if (server_check_timer_) {
      server_check_timer_->cancel();
    }

    if (server_check_thread_.joinable()) {
      server_check_thread_.join();
    }
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool bound_;
  std::string bound_path_;

  std::vector<uint8_t> buffer_;

  std::thread server_check_thread_;
  std::atomic<bool> server_check_enabled_;
  std::shared_ptr<thread_utility::timer> server_check_timer_;
};
} // namespace local_datagram
} // namespace krbn
