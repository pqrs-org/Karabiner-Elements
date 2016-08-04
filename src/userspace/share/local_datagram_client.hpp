#pragma once

class local_datagram_client final {
public:
  local_datagram_client(const char* _Nonnull path) : endpoint_(path),
                                                     io_service_(),
                                                     work_(io_service_),
                                                     socket_(io_service_) {
    socket_.open();
    thread_ = std::thread([this] { (this->io_service_).run(); });
  }

  ~local_datagram_client(void) {
    io_service_.stop();
    thread_.join();
  }

  void send_to(const uint8_t* _Nonnull buffer, size_t buffer_length) {
    auto ptr = std::make_shared<std::vector<uint8_t>>();
    ptr->resize(buffer_length);
    memcpy(&((*ptr)[0]), buffer, buffer_length);
    io_service_.post(boost::bind(&local_datagram_client::do_send, this, ptr));
  }

private:
  void do_send(const std::shared_ptr<std::vector<uint8_t>>& ptr) {
    socket_.async_send_to(boost::asio::buffer(*ptr), endpoint_,
                          boost::bind(&local_datagram_client::handle_send, this, ptr));
  }

  void handle_send(const std::shared_ptr<std::vector<uint8_t>>& ptr) {
    // shared_ptr will be released.
  }

  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::io_service::work work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread thread_;
};
