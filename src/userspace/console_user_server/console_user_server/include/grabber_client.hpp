#pragma once

class grabber_client final {
public:
  void open(void) {
    io_service_ = std::make_unique<boost::asio::io_service>();
    boost::asio::local::datagram_protocol::endpoint ep("/tmp/karabiner_grabber");
    socket_ = std::make_unique<boost::asio::local::datagram_protocol::socket>(*io_service_);

    char request[4];
    request[0] = 0xde;
    request[1] = 0xad;
    request[2] = 0xbe;
    request[3] = 0xef;

    socket_->open();
    socket_->send_to(boost::asio::buffer(request, sizeof(request)), ep);
    std::cout << "sent" << std::endl;
  }

  void stop(void) {
  }

private:
  std::unique_ptr<boost::asio::io_service> io_service_;
  std::unique_ptr<boost::asio::local::datagram_protocol::socket> socket_;
};
