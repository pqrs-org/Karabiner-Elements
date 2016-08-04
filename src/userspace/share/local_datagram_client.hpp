#pragma once

#include <mutex>

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
    std::lock_guard<std::mutex> lock(mutex_);

    auto item_ptr = std::make_shared<item>(buffer, buffer_length);
    queue_.push_back(item_ptr);
    io_service_.post(boost::bind(&local_datagram_client::do_send, this, boost::ref(*item_ptr)));
  }

private:
  class item final {
  public:
    item(const uint8_t* _Nonnull buffer, size_t buffer_length) : id_(get_next_id()) {
      buffer_.resize(buffer_length);
      memcpy(&(buffer_[0]), buffer, buffer_length);
    }

    static uint64_t get_next_id(void) {
      static uint64_t id;
      return ++id;
    }

    uint64_t get_id(void) const { return id_; }
    const std::vector<uint8_t> get_buffer(void) const { return buffer_; }

  private:
    uint64_t id_;
    std::vector<uint8_t> buffer_;
  };

  void do_send(const item& item) {
    socket_.async_send_to(boost::asio::buffer(item.get_buffer()), endpoint_,
                          boost::bind(&local_datagram_client::handle_send, this, item.get_id()));
  }

  void handle_send(uint64_t item_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& i : queue_) {
      if (i->get_id() == item_id) {
        queue_.remove(i);
        return;
      }
    }
  }

  boost::asio::local::datagram_protocol::endpoint endpoint_;
  boost::asio::io_service io_service_;
  boost::asio::io_service::work work_;
  boost::asio::local::datagram_protocol::socket socket_;
  std::thread thread_;
  std::mutex mutex_;
  std::list<std::shared_ptr<item>> queue_;
};
