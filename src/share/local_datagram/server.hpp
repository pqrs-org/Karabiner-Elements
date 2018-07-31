#pragma once

#include "boost_defs.hpp"

BEGIN_BOOST_INCLUDE
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
END_BOOST_INCLUDE

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
  boost::signals2::signal<void(const boost::asio::const_buffer&)> received;

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
    close();

    if (io_service_thread_.joinable()) {
      work_ = nullptr;
      io_service_thread_.join();
    }
  }

  void bind(const std::string& path,
            size_t buffer_size) {
    close();

    io_service_.post([this, path, buffer_size] {
      bound_ = false;
      bound_path_.clear();

      // open

      {
        boost::system::error_code error_code;
        socket_.open(boost::asio::local::datagram_protocol::socket::protocol_type(),
                     error_code);
        if (error_code) {
          bind_failed(error_code);
          return;
        }
      }

      // bind

      {
        boost::system::error_code error_code;
        socket_.bind(boost::asio::local::datagram_protocol::endpoint(path),
                     error_code);

        if (error_code) {
          bind_failed(error_code);
          return;
        }
      }

      bound_ = true;
      bound_path_ = path;

      bound();

      // async_receive

      buffer_.resize(buffer_size);
      async_receive();
    });
  }

  void close(void) {
    io_service_.post([this] {
      boost::system::error_code error_code;

      socket_.cancel(error_code);
      socket_.close(error_code);

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

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread io_service_thread_;
  bool bound_;
  std::string bound_path_;

  std::vector<uint8_t> buffer_;
};
} // namespace local_datagram
} // namespace krbn
