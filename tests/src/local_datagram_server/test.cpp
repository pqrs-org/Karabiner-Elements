#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "filesystem.hpp"
#include "local_datagram_client.hpp"
#include "local_datagram_server.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

namespace {
const std::string socket_path("tmp/server.sock");
const std::chrono::milliseconds client_heartbeat_interval(100);
size_t server_count = 0;
size_t server_receive_count = 0;

void reset_count(void) {
  server_count = 0;
  server_receive_count = 0;
}

class test_server final {
public:
  test_server(void) : exit_loop_(false) {
    unlink(socket_path.c_str());

    reset_count();

    server_ = std::make_unique<krbn::local_datagram_server>(socket_path);

    thread_ = std::thread([this] {
      while (!exit_loop_) {
        ++server_count;

        boost::system::error_code ec;
        std::vector<uint8_t> buffer(1024);

        std::size_t n = server_->receive(boost::asio::buffer(buffer),
                                         boost::posix_time::milliseconds(100),
                                         ec);

        if (!ec) {
          krbn::logger::get_logger().info("server received {0}", n);
          server_receive_count += n;
        }
      }
    });
  }

  ~test_server(void) {
    exit_loop_ = true;
    thread_.join();
  }

private:
  std::atomic<bool> exit_loop_;
  std::unique_ptr<krbn::local_datagram_server> server_;
  std::thread thread_;
};

class test_client final {
public:
  test_client(void) : closed_(false) {
    client_ = std::make_unique<krbn::local_datagram_client>(socket_path);

    client_->connected.connect([this](void) {
      connected_ = true;

      client_->start_heartbeat(client_heartbeat_interval);
    });

    client_->connect_failed.connect([this](auto&& error_code) {
      connected_ = false;

      std::cout << error_code.message() << std::endl;
    });

    client_->closed.connect([this](void) {
      closed_ = true;
    });

    client_->connect();
  }

  ~test_client(void) {
    client_ = nullptr;
  }

  boost::optional<bool> get_connected(void) const {
    return connected_;
  }

  bool get_closed(void) const {
    return closed_;
  }

  void async_send(void) {
    std::vector<uint8_t> client_buffer(32);
    client_->async_send(client_buffer);
  }

private:
  boost::optional<bool> connected_;
  bool closed_;
  std::unique_ptr<krbn::local_datagram_client> client_;
};
} // namespace

TEST_CASE("socket file") {
  unlink(socket_path.c_str());
  REQUIRE(!krbn::filesystem::exists(socket_path));

  {
    krbn::local_datagram_server server(socket_path);
    REQUIRE(krbn::filesystem::exists(socket_path));
  }

  REQUIRE(!krbn::filesystem::exists(socket_path));
}

TEST_CASE("fail to create socket file") {
  REQUIRE_THROWS(krbn::local_datagram_server("/not_found/server.sock"));
}

TEST_CASE("keep existing file in destructor") {
  std::string regular_file_path("tmp/regular_file");

  {
    std::ofstream file(regular_file_path);
    file << regular_file_path << std::endl;
  }

  REQUIRE(krbn::filesystem::exists(regular_file_path));

  {
    REQUIRE_THROWS(krbn::local_datagram_server(regular_file_path));
  }

  REQUIRE(krbn::filesystem::exists(regular_file_path));
}

TEST_CASE("permission error") {
  auto server = std::make_unique<test_server>();

  chmod(socket_path.c_str(), 0000);

  auto client = std::make_unique<test_client>();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  REQUIRE(client->get_connected() == false);
}

TEST_CASE("local_datagram_server") {
  {
    auto server = std::make_unique<test_server>();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto client = std::make_unique<test_client>();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(client->get_connected() == true);

    client->async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!client->get_closed());

    REQUIRE(server_count > 5);
    REQUIRE(server_receive_count == 32);

    // Destroy server

    server = nullptr;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(client->get_closed());
  }

  // Send after server is destroyed.
  {
    reset_count();

    REQUIRE(!krbn::filesystem::exists(socket_path));

    test_client client;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(client.get_connected() == false);

    client.async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!client.get_closed());

    REQUIRE(server_receive_count == 0);
  }

  // Create client before server
  {
    test_client client;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(client.get_connected() == false);

    test_server server;

    REQUIRE(server_receive_count == 0);

    client.async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(server_receive_count == 0);
  }
}
