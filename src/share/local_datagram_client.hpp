#pragma once

#include "boost_defs.hpp"

BEGIN_BOOST_INCLUDE
#include <boost/asio.hpp>
#include <boost/bind.hpp>
END_BOOST_INCLUDE

#include <spdlog/spdlog.h>
#include <thread>

namespace krbn {
class local_datagram_client final {
public:
  local_datagram_client(const local_datagram_client&) = delete;

  local_datagram_client(spdlog::logger& logger,
                        const char* _Nonnull path) : logger_(logger),
                                                     endpoint_(path),
                                                     io_service_(),
                                                     work_(io_service_),
                                                     socket_(io_service_) {
    socket_.open();
    thread_ = std::thread([this] { (this->io_service_).run(); });
  }

  ~local_datagram_client(void) {
    io_service_.post(boost::bind(&local_datagram_client::do_stop, this));
    thread_.join();
  }

  void send_to(const std::vector<uint8_t>& v) {
    auto ptr = std::make_shared<buffer>(v);
    io_service_.post(boost::bind(&local_datagram_client::do_send, this, ptr));
  }

  void send_to(const uint8_t* _Nonnull p, size_t length) {
    auto ptr = std::make_shared<buffer>(p, length);
    io_service_.post(boost::bind(&local_datagram_client::do_send, this, ptr));
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

  void do_send(const std::shared_ptr<buffer>& ptr) {
    socket_.async_send_to(boost::asio::buffer(ptr->get_vector()),
                          endpoint_,
                          boost::bind(&local_datagram_client::handle_send,
                                      this,
                                      boost::asio::placeholders::error,
                                      ptr));
  }

  void handle_send(const boost::system::error_code& ec,
                   const std::shared_ptr<buffer>& ptr) {
    // buffer will be released.
    if (ec) {
      logger_.error("local_datagram_client error: {0}", ec.message());
    }
  }

  void do_stop(void) {
    io_service_.stop();
  }

  spdlog::logger& logger_;

  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::io_service::work work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread thread_;
};
} // namespace krbn
