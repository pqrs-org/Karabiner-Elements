#pragma once

// `krbn::local_datagram::server` can be used safely in a multi-threaded environment.

#include "local_datagram/client.hpp"
#include <unistd.h>

namespace krbn {
namespace local_datagram {
class server final {
public:
  // Signals

  boost::signals2::signal<void(void)> bound;
  boost::signals2::signal<void(const boost::system::error_code&)> bind_failed;
  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(const std::shared_ptr<std::vector<uint8_t>>)> received;

  // Methods

  server(const server&) = delete;

  server(void) : io_service_(),
                 work_(std::make_unique<boost::asio::io_service::work>(io_service_)),
                 socket_(io_service_),
                 bound_(false) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  ~server(void) {
    async_close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }
  }

  void async_bind(const std::string& path,
                  size_t buffer_size,
                  boost::optional<std::chrono::milliseconds> server_check_interval) {
    async_close();

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

      start_server_check_timer(path,
                               server_check_interval);

      bound();

      // Receive

      buffer_.resize(buffer_size);
      async_receive();
    });
  }

  void async_close(void) {
    io_service_.post([this] {
      close();
    });
  }

private:
  void close(void) {
    stop_server_check_timer();

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
  }

  void async_receive(void) {
    socket_.async_receive(boost::asio::buffer(buffer_),
                          [this](auto&& error_code, auto&& bytes_transferred) {
                            if (!error_code) {
                              auto v = std::make_shared<std::vector<uint8_t>>(bytes_transferred);
                              std::copy(std::begin(buffer_),
                                        std::begin(buffer_) + bytes_transferred,
                                        std::begin(*v));
                              received(v);
                            }

                            // receive once if not closed

                            if (bound_) {
                              async_receive();
                            }
                          });
  }

  void start_server_check_timer(const std::string& path,
                                boost::optional<std::chrono::milliseconds> server_check_interval) {
    if (!server_check_interval) {
      return;
    }

    if (server_check_timer_) {
      return;
    }

    server_check_timer_ = std::make_unique<thread_utility::timer>(
        *server_check_interval,
        thread_utility::timer::mode::repeat,
        [this, path] {
          io_service_.post([this, path] {
            // Skip if client is connecting
            if (server_check_client_) {
              return;
            }

            server_check_client_ = std::make_unique<client>();

            server_check_client_->connected.connect([this] {
              io_service_.post([this] {
                server_check_client_ = nullptr;
              });
            });

            server_check_client_->connect_failed.connect([this](auto&& error_code) {
              io_service_.post([this] {
                close();
              });
            });

            server_check_client_->async_connect(path, boost::none);
          });
        });
  }

  void stop_server_check_timer(void) {
    if (!server_check_timer_) {
      return;
    }

    server_check_timer_->cancel();
    server_check_timer_ = nullptr;

    server_check_client_ = nullptr;
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool bound_;
  std::string bound_path_;

  std::vector<uint8_t> buffer_;

  std::unique_ptr<thread_utility::timer> server_check_timer_;
  std::unique_ptr<client> server_check_client_;
};
} // namespace local_datagram
} // namespace krbn
