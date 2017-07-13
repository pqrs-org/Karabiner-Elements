#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "local_datagram_server.hpp"
#include "types.hpp"
#include <vector>

namespace krbn {
class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) : exit_loop_(false),
                   shell_command_execution_queue_(nullptr) {
    const size_t buffer_length = 32 * 1024;
    buffer_.resize(buffer_length);

    auto socket_file_path = console_user_server_client::make_console_user_server_socket_file_path(getuid());

    auto path = socket_file_path.c_str();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    chmod(path, 0600);

    shell_command_execution_queue_ = dispatch_queue_create("console_user_server_receiver", nullptr);

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });

    logger::get_logger().info("receiver is initialized");
  }

  ~receiver(void) {
    dispatch_release(shell_command_execution_queue_);
    shell_command_execution_queue_ = nullptr;

    unlink(socket_path_.c_str());

    exit_loop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }

    server_ = nullptr;

    logger::get_logger().info("receiver is terminated");
  }

private:
  void worker(void) {
    if (!server_) {
      return;
    }

    while (!exit_loop_) {
      boost::system::error_code ec;
      std::size_t n = server_->receive(boost::asio::buffer(buffer_), boost::posix_time::seconds(1), ec);

      if (!ec && n > 0) {
        switch (operation_type(buffer_[0])) {
          case operation_type::shell_command_execution:
            if (n != sizeof(operation_type_shell_command_execution_struct)) {
              logger::get_logger().error("invalid size for operation_type::shell_command_execution");
            } else {
              auto p = reinterpret_cast<operation_type_shell_command_execution_struct*>(&(buffer_[0]));

              // Ensure shell_command is null-terminated string even if corrupted data is sent.
              p->shell_command[sizeof(p->shell_command) - 1] = '\0';

              std::string shell_command = p->shell_command;
              if (shell_command_execution_queue_) {
                dispatch_async(shell_command_execution_queue_, ^{
                  system(shell_command.c_str());
                });
              }
            }
            break;

          default:
            break;
        }
      }
    }
  }

  std::string socket_path_;
  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::thread thread_;
  std::atomic<bool> exit_loop_;

  dispatch_queue_t shell_command_execution_queue_;
};
} // namespace krbn
