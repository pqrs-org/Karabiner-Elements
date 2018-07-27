#pragma once

#include "boost_defs.hpp"

BEGIN_BOOST_INCLUDE
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
END_BOOST_INCLUDE

#include "logger.hpp"
#include <atomic>
#include <chrono>
#include <thread>

namespace krbn {
class local_datagram_client final {
public:
  // Signals

  // Note: These signals are fired on non-main thread.

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  local_datagram_client(const local_datagram_client&) = delete;

  local_datagram_client(void) : io_service_(),
                                work_(std::make_unique<boost::asio::io_service::work>(io_service_)),
                                socket_(io_service_),
                                connected_(false),
                                heartbeat_enabled_(false) {
    io_service_thread_ = std::thread([this] {
      (this->io_service_).run();
    });
  }

  ~local_datagram_client(void) {
    close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }
  }

  void connect(const std::string& path) {
    close();

    io_service_.post([this, path] {
      connected_ = false;

      // open

      {
        boost::system::error_code error_code;
        socket_.open(boost::asio::local::datagram_protocol::socket::protocol_type(),
                     error_code);
        if (error_code) {
          connect_failed(error_code);
          return;
        }
      }

      // async_connect

      socket_.async_connect(boost::asio::local::datagram_protocol::endpoint(path),
                            [this](auto&& error_code) {
                              if (error_code) {
                                connect_failed(error_code);
                              } else {
                                connected_ = true;
                                connected();
                              }
                            });
    });
  }

  void close(void) {
    stop_heartbeat();

    io_service_.post([this] {
      boost::system::error_code error_code;

      socket_.cancel(error_code);
      socket_.close(error_code);

      if (connected_) {
        connected_ = false;
        closed();
      }
    });
  }

  void start_heartbeat(std::chrono::milliseconds heartbeat_interval) {
    stop_heartbeat();

    io_service_.post([this, heartbeat_interval] {
      heartbeat_enabled_ = true;
      heartbeat_thread_ = std::thread([this, heartbeat_interval] {
        std::vector<uint8_t> data;
        while (heartbeat_enabled_) {
          std::this_thread::sleep_for(heartbeat_interval);

          async_send(data);
        }
      });
    });
  }

  void stop_heartbeat(void) {
    io_service_.post([this] {
      heartbeat_enabled_ = false;

      if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
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
                           close();
                         }
                       });
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool connected_;

  std::thread heartbeat_thread_;
  std::atomic<bool> heartbeat_enabled_;
};
} // namespace krbn
