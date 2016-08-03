#pragma once

class local_datagram_client final {
public:
  local_datagram_client(const char* _Nonnull path) : endpoint_(path),
                                                     io_service_(),
                                                     socket_(io_service_) {
  }

private:
  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::local::datagram_protocol::socket socket_;
};
