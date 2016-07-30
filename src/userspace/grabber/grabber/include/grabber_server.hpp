#pragma once

class grabber_server final {
public:
  void start(void) {
    std::thread(worker);
  }

  void stop(void) {
  }

  void worker(void) {
    unlink("/tmp/karabiner_grabber");
    asio::local::datagram_protocol::endpoint ep("/tmp/karabiner_grabber");
    asio::io_service io_service;
    asio::local::datagram_protocol::socket socket(io_service, ep);

    for (;;)
    {
      asio::local::datagram_protocol::endpoint sender_endpoint;
      size_t length = socket.receive_from(asio::buffer(buffer_), sender_endpoint);
      std::cout << length << std::endl;
    }
  }

private:
  enum {
    buffer_length = 8 * 1024 * 1024,
  };
  std::array<uint8_t, buffer_length> buffer_;
};
