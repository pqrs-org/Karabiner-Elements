#pragma once

#include "boost_defs.hpp"

BEGIN_BOOST_INCLUDE
#include <boost/asio.hpp>
#include <boost/bind.hpp>
END_BOOST_INCLUDE

#include <unistd.h>

namespace krbn {
class local_datagram_server final {
public:
  // Signals

  // Note: These signals are fired on non-main thread.

  // Methods

  local_datagram_server(const local_datagram_server&) = delete;

  local_datagram_server(const std::string& path) : endpoint_(path),
                                                   io_service_(),
                                                   socket_(io_service_, endpoint_),
                                                   deadline_(io_service_) {
    deadline_.expires_at(boost::posix_time::pos_infin);
    check_deadline();
  }

  ~local_datagram_server(void) {
    unlink(endpoint_.path().c_str());
  }

  // from doc/html/boost_asio/example/cpp03/timeouts/blocking_udp_client.cpp
  std::size_t receive(const boost::asio::mutable_buffer& buffer,
                      boost::posix_time::time_duration timeout,
                      boost::system::error_code& ec) {
    deadline_.expires_from_now(timeout);

    ec = boost::asio::error::would_block;
    std::size_t length = 0;

    socket_.async_receive(boost::asio::buffer(buffer),
                          boost::bind(&local_datagram_server::handle_receive, _1, _2, &ec, &length));

    do {
      io_service_.run_one();
    } while (ec == boost::asio::error::would_block);

    return length;
  }

private:
  void check_deadline(void) {
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
      socket_.cancel();
      deadline_.expires_at(boost::posix_time::pos_infin);
    }

    deadline_.async_wait(boost::bind(&local_datagram_server::check_deadline, this));
  }

  static void handle_receive(const boost::system::error_code& ec,
                             std::size_t length,
                             boost::system::error_code* _Nonnull out_ec,
                             std::size_t* _Nonnull out_length) {
    *out_ec = ec;
    *out_length = length;
  }

  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::local::datagram_protocol::socket socket_;
  boost::asio::deadline_timer deadline_;
};
} // namespace krbn
