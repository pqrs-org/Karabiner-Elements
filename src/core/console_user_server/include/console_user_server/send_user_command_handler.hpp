#pragma once

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include <asio.hpp>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <string>
#include <sys/un.h>
#include <thread>

namespace krbn {
namespace console_user_server {

class send_user_command_handler final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  send_user_command_handler(const send_user_command_handler&) = delete;

  send_user_command_handler(void)
      : dispatcher_client(),
        io_context_(),
        work_guard_(asio::make_work_guard(io_context_)),
        socket_(io_context_) {
    io_thread_ = std::thread([this] {
      io_context_.run();
    });
  }

  ~send_user_command_handler(void) {
    detach_from_dispatcher();

    asio::post(io_context_, [this] {
      close_socket();
    });

    work_guard_.reset();

    if (io_thread_.joinable()) {
      io_thread_.join();
    }
  }

  void run(const nlohmann::json& user_command) {
    enqueue_to_dispatcher([this, user_command] {
      try {
        auto endpoint = (constants::get_system_user_directory(geteuid()) / "user_command_receiver.sock").string();
        if (user_command.contains("endpoint")) {
          endpoint = user_command.at("endpoint").get<std::string>();
        }

        if (!user_command.contains("payload")) {
          logger::get_logger()->error("send_user_command: missing `payload`");
          return;
        }
        auto payload = user_command.at("payload").dump();

        auto payload_ptr = std::make_shared<std::string>(std::move(payload));
        asio::post(io_context_, [this, endpoint, payload_ptr] {
          if (!send_datagram(endpoint, *payload_ptr)) {
            logger::get_logger()->warn("send_user_command: send failed to {}", endpoint);
          }
        });

      } catch (const std::exception& e) {
        logger::get_logger()->error("send_user_command error: {}", e.what());
      }
    });
  }

private:
  bool ensure_socket_open(void) {
    if (socket_.is_open()) {
      return true;
    }

    asio::error_code error_code;
    socket_.open(asio::local::datagram_protocol(), error_code);
    if (error_code) {
      logger::get_logger()->warn("send_user_command: socket.open failed: {}", error_code.message());
      return false;
    }

    auto path = constants::get_user_tmp_directory() / filesystem_utility::make_socket_file_basename();
    socket_file_path_ = path;

    std::error_code ec;
    std::filesystem::remove(path, ec);

    asio::local::datagram_protocol::endpoint local_endpoint(path.string());
    socket_.bind(local_endpoint, error_code);
    if (error_code) {
      logger::get_logger()->warn("send_user_command: socket.bind failed: {}", error_code.message());
      socket_.close();
      return false;
    }

    return true;
  }

  bool send_datagram(const std::string& endpoint,
                     const std::string& payload) {
    if (endpoint.size() >= sizeof(((struct sockaddr_un*)nullptr)->sun_path)) {
      logger::get_logger()->warn("send_user_command: endpoint path too long: {}", endpoint);
      return false;
    }

    if (!ensure_socket_open()) {
      return false;
    }

    asio::error_code error_code;
    asio::local::datagram_protocol::endpoint destination(endpoint);
    auto bytes_sent = socket_.send_to(asio::buffer(payload), destination, 0, error_code);
    if (error_code) {
      logger::get_logger()->warn("send_user_command: send_to failed: {}", error_code.message());
      return false;
    }

    return bytes_sent == payload.size();
  }

  void close_socket(void) {
    if (socket_.is_open()) {
      socket_.close();
    }

    if (socket_file_path_) {
      std::error_code ec;
      std::filesystem::remove(*socket_file_path_, ec);
      socket_file_path_.reset();
    }
  }

  asio::io_context io_context_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  asio::local::datagram_protocol::socket socket_;
  std::optional<std::filesystem::path> socket_file_path_;
  std::thread io_thread_;
};

} // namespace console_user_server
} // namespace krbn
