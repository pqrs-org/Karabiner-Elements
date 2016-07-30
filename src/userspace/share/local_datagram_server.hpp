#pragma once

class local_datagram_server final {
public:
  local_datagram_server(const char* _Nonnull path) : endpoint_(path),
                                                     io_service_(),
                                                     socket_(io_service_, endpoint_) {}

  asio::local::datagram_protocol::socket& get_socket(void) { return socket_; }

private:
  asio::local::datagram_protocol::endpoint endpoint_;
  asio::io_service io_service_;
  asio::local::datagram_protocol::socket socket_;
};
