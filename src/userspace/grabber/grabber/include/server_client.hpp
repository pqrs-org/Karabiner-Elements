#pragma once

class server_client final {
public:
  server_client(const char* _Nonnull path) : endpoint_(path),
                                             io_service_(),
                                             socket_(io_service_, endpoint_) {
    socket_.open();
  }

  void send(const uint8_t* _Nonnull buffer, size_t buffer_length) {
    socket_.send_to(boost::asio::buffer(buffer, buffer_length), endpoint_);
  }

private:
  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::local::datagram_protocol::socket socket_;
};
