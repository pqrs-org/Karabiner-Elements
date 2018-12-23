#pragma once

// `krbn::local_datagram::client` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

BEGIN_BOOST_INCLUDE
#include <boost/asio.hpp>
END_BOOST_INCLUDE

#include "logger.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace local_datagram {
class client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const boost::system::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  client(const client&) = delete;

  client(void) : dispatcher_client(),
                 io_service_(),
                 work_(std::make_unique<boost::asio::io_service::work>(io_service_)),
                 socket_(io_service_),
                 connected_(false),
                 server_check_timer_(*this) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  virtual ~client(void) {
    async_close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }

    detach_from_dispatcher([] {
    });
  }

  void async_connect(const std::string& path,
                     std::optional<std::chrono::milliseconds> server_check_interval) {
    async_close();

    io_service_.post([this, path, server_check_interval] {
      connected_ = false;

      // Open

      {
        boost::system::error_code error_code;
        socket_.open(boost::asio::local::datagram_protocol::socket::protocol_type(),
                     error_code);
        if (error_code) {
          enqueue_to_dispatcher([this, error_code] {
            connect_failed(error_code);
          });
          return;
        }
      }

      // Connect

      socket_.async_connect(boost::asio::local::datagram_protocol::endpoint(path),
                            [this, server_check_interval](auto&& error_code) {
                              if (error_code) {
                                enqueue_to_dispatcher([this, error_code] {
                                  connect_failed(error_code);
                                });
                              } else {
                                connected_ = true;

                                stop_server_check();
                                start_server_check(server_check_interval);

                                enqueue_to_dispatcher([this] {
                                  connected();
                                });
                              }
                            });
    });
  }

  void async_close(void) {
    io_service_.post([this] {
      stop_server_check();

      // Close socket

      boost::system::error_code error_code;

      socket_.cancel(error_code);
      socket_.close(error_code);

      // Signal

      if (connected_) {
        connected_ = false;

        enqueue_to_dispatcher([this] {
          closed();
        });
      }
    });
  }

  void async_send(const std::vector<uint8_t>& v) {
    auto ptr = std::make_shared<buffer>(v);
    io_service_.post([this, ptr] {
      do_async_send(ptr);
    });
  }

  void async_send(const uint8_t* _Nonnull p, size_t length) {
    auto ptr = std::make_shared<buffer>(p, length);
    io_service_.post([this, ptr] {
      do_async_send(ptr);
    });
  }

private:
  class buffer final {
  public:
    buffer(const std::vector<uint8_t> v) {
      v_ = v;
    }

    buffer(const uint8_t* _Nonnull p, size_t length) {
      v_.resize(length);
      memcpy(&(v_[0]), p, length);
    }

    const std::vector<uint8_t>& get_vector(void) const { return v_; }

  private:
    std::vector<uint8_t> v_;
  };

  void do_async_send(std::shared_ptr<buffer> ptr) {
    socket_.async_send(boost::asio::buffer(ptr->get_vector()),
                       [this, ptr](auto&& error_code,
                                   auto&& bytes_transferred) {
                         if (error_code) {
                           async_close();
                         }
                       });
  }

  // This method is executed in `io_service_thread_`.
  void start_server_check(std::optional<std::chrono::milliseconds> server_check_interval) {
    if (server_check_interval) {
      server_check_timer_.start(
          [this] {
            io_service_.post([this] {
              check_server();
            });
          },
          *server_check_interval);
    }
  }

  // This method is executed in `io_service_thread_`.
  void stop_server_check(void) {
    server_check_timer_.stop();
  }

  // This method is executed in `io_service_thread_`.
  void check_server(void) {
    std::vector<uint8_t> data;
    async_send(data);
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool connected_;

  pqrs::dispatcher::extra::timer server_check_timer_;
};
} // namespace local_datagram
} // namespace krbn
