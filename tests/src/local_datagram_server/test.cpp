#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "local_datagram/client.hpp"
#include "local_datagram/client_manager.hpp"
#include "local_datagram/server.hpp"
#include "local_datagram/server_manager.hpp"
#include <pqrs/filesystem.hpp>

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

namespace {
const std::string socket_path("tmp/server.sock");
size_t server_buffer_size(32 * 1024);
const std::chrono::milliseconds server_check_interval(100);

class test_server final {
public:
  test_server(void) : closed_(false),
                      received_count_(0) {
    unlink(socket_path.c_str());

    server_ = std::make_unique<krbn::local_datagram::server>();

    server_->bound.connect([this] {
      bound_ = true;
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      bound_ = false;
    });

    server_->closed.connect([this] {
      closed_ = true;
    });

    server_->received.connect([this](auto&& buffer) {
      krbn::logger::get_logger()->info("server received {0}", buffer->size());
      received_count_ += buffer->size();

      if (buffer->size() == 32) {
        REQUIRE((*buffer)[0] == 10);
        REQUIRE((*buffer)[1] == 20);
        REQUIRE((*buffer)[2] == 30);
      }
    });

    server_->async_bind(socket_path,
                        server_buffer_size,
                        server_check_interval);

    // Wait server initialization roughly

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  ~test_server(void) {
    server_ = nullptr;
  }

  std::optional<bool> get_bound(void) const {
    return bound_;
  }

  bool get_closed(void) const {
    return closed_;
  }

  size_t get_received_count(void) const {
    return received_count_;
  }

private:
  std::optional<bool> bound_;
  bool closed_;
  size_t received_count_;
  std::unique_ptr<krbn::local_datagram::server> server_;
};

class test_client final {
public:
  test_client(void) : closed_(false) {
    client_ = std::make_unique<krbn::local_datagram::client>();

    client_->connected.connect([this] {
      connected_ = true;
    });

    client_->connect_failed.connect([this](auto&& error_code) {
      connected_ = false;

      std::cout << error_code.message() << std::endl;
    });

    client_->closed.connect([this] {
      closed_ = true;
    });

    client_->async_connect(socket_path,
                           server_check_interval);

    // Wait client initialization roughly

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  ~test_client(void) {
    client_ = nullptr;
  }

  std::optional<bool> get_connected(void) const {
    return connected_;
  }

  bool get_closed(void) const {
    return closed_;
  }

  void async_send(void) {
    std::vector<uint8_t> client_buffer(32);
    client_buffer[0] = 10;
    client_buffer[1] = 20;
    client_buffer[2] = 30;
    if (client_) {
      client_->async_send(client_buffer);
    }
  }

private:
  std::optional<bool> connected_;
  bool closed_;
  std::unique_ptr<krbn::local_datagram::client> client_;
};
} // namespace

TEST_CASE("socket file") {
  unlink(socket_path.c_str());
  REQUIRE(!pqrs::filesystem::exists(socket_path));

  {
    krbn::local_datagram::server server;

    server.async_bind(socket_path,
                      server_buffer_size,
                      server_check_interval);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(pqrs::filesystem::exists(socket_path));
  }

  REQUIRE(!pqrs::filesystem::exists(socket_path));
}

TEST_CASE("fail to create socket file") {
  krbn::local_datagram::server server;

  bool failed = false;

  server.bind_failed.connect([&](auto&& error_code) {
    failed = true;
  });

  server.async_bind("/not_found/server.sock",
                    server_buffer_size,
                    server_check_interval);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  REQUIRE(failed == true);
}

TEST_CASE("keep existing file in destructor") {
  std::string regular_file_path("tmp/regular_file");

  {
    std::ofstream file(regular_file_path);
    file << regular_file_path << std::endl;
  }

  REQUIRE(pqrs::filesystem::exists(regular_file_path));

  {
    krbn::local_datagram::server server;
    server.async_bind(regular_file_path,
                      server_buffer_size,
                      server_check_interval);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  REQUIRE(pqrs::filesystem::exists(regular_file_path));
}

TEST_CASE("permission error") {
  {
    auto server = std::make_unique<test_server>();

    // ----
    chmod(socket_path.c_str(), 0000);

    {
      auto client = std::make_unique<test_client>();
      REQUIRE(client->get_connected() == false);
    }

    REQUIRE(server->get_closed());
  }

  {
    auto server = std::make_unique<test_server>();

    // -r--
    chmod(socket_path.c_str(), 0400);

    {
      auto client = std::make_unique<test_client>();
      REQUIRE(client->get_connected() == false);
    }

    REQUIRE(server->get_closed());
  }

  {
    auto server = std::make_unique<test_server>();

    // -rw-
    chmod(socket_path.c_str(), 0600);

    {
      auto client = std::make_unique<test_client>();
      REQUIRE(client->get_connected() == true);
    }

    REQUIRE(!server->get_closed());
  }
}

TEST_CASE("close when socket erased") {
  auto server = std::make_unique<test_server>();

  unlink(socket_path.c_str());

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  REQUIRE(server->get_closed());
}

TEST_CASE("local_datagram::server") {
  {
    auto server = std::make_unique<test_server>();
    auto client = std::make_unique<test_client>();

    REQUIRE(client->get_connected() == true);

    client->async_send();
    client->async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!client->get_closed());

    REQUIRE(server->get_received_count() == 64);

    // Destroy server

    server = nullptr;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(client->get_closed());
  }

  // Send after server is destroyed.
  {
    REQUIRE(!pqrs::filesystem::exists(socket_path));

    test_client client;

    REQUIRE(client.get_connected() == false);

    client.async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!client.get_closed());
  }

  // Create client before server
  {
    test_client client;

    REQUIRE(client.get_connected() == false);

    test_server server;

    REQUIRE(server.get_received_count() == 0);

    client.async_send();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(server.get_received_count() == 0);
  }
}

TEST_CASE("local_datagram::client_manager") {
  {
    size_t connected_count = 0;
    size_t connect_failed_count = 0;
    size_t closed_count = 0;

    std::chrono::milliseconds reconnect_interval(100);

    auto client_manager = std::make_unique<krbn::local_datagram::client_manager>(socket_path,
                                                                                 server_check_interval,
                                                                                 reconnect_interval);

    client_manager->connected.connect([&] {
      ++connected_count;
      krbn::logger::get_logger()->info("client_manager connected: {0}", connected_count);
    });

    client_manager->connect_failed.connect([&](auto&& error_code) {
      ++connect_failed_count;
      krbn::logger::get_logger()->info("client_manager connect_failed: {0}", connect_failed_count);
    });

    client_manager->closed.connect([&] {
      ++closed_count;
      krbn::logger::get_logger()->info("client_manager closed: {0}", closed_count);
    });

    // Create client before server

    client_manager->async_start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(connected_count == 0);
    REQUIRE(connect_failed_count > 2);
    REQUIRE(closed_count == 0);

    // Create server

    auto server = std::make_unique<test_server>();

    REQUIRE(connected_count == 1);

    // Shtudown servr

    connect_failed_count = 0;

    server = nullptr;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(connected_count == 1);
    REQUIRE(connect_failed_count > 2);
    REQUIRE(closed_count == 1);

    // Recreate server

    server = std::make_unique<test_server>();

    REQUIRE(connected_count == 2);
  }
}

TEST_CASE("local_datagram::server_manager") {
  {
    size_t bound_count = 0;
    size_t bind_failed_count = 0;
    size_t closed_count = 0;

    std::chrono::milliseconds reconnect_interval(100);

    auto server_manager = std::make_unique<krbn::local_datagram::server_manager>(socket_path,
                                                                                 server_buffer_size,
                                                                                 server_check_interval,
                                                                                 reconnect_interval);

    server_manager->bound.connect([&] {
      ++bound_count;
      krbn::logger::get_logger()->info("server_manager bound: {0}", bound_count);
    });

    server_manager->bind_failed.connect([&](auto&& error_code) {
      ++bind_failed_count;
      krbn::logger::get_logger()->info("server_manager bind_failed: {0}", bind_failed_count);
    });

    server_manager->closed.connect([&] {
      ++closed_count;
      krbn::logger::get_logger()->info("server_manager closed: {0}", closed_count);
    });

    server_manager->async_start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(bound_count == 1);
    REQUIRE(bind_failed_count == 0);
    REQUIRE(closed_count == 0);

    unlink(socket_path.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(bound_count == 2);
    REQUIRE(bind_failed_count == 0);
    REQUIRE(closed_count == 1);
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
