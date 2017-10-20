#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "input_source_manager.hpp"
#include "local_datagram_server.hpp"
#include "shell_utility.hpp"
#include "types.hpp"
#include <vector>

namespace krbn {
class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) : exit_loop_(false),
                   last_select_input_source_time_stamp_(0) {
    const size_t buffer_length = 32 * 1024;
    buffer_.resize(buffer_length);

    auto socket_file_path = console_user_server_client::make_console_user_server_socket_file_path(getuid());

    auto path = socket_file_path.c_str();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    chmod(path, 0600);

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });

    logger::get_logger().info("receiver is initialized");
  }

  ~receiver(void) {
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

              std::string background_shell_command = shell_utility::make_background_command(p->shell_command);
              dispatch_async(dispatch_get_main_queue(), ^{
                system(background_shell_command.c_str());
              });
            }
            break;

          case operation_type::select_input_source:
            if (n != sizeof(operation_type_select_input_source_struct)) {
              logger::get_logger().error("invalid size for operation_type::select_input_source");
            } else {
              auto p = reinterpret_cast<operation_type_select_input_source_struct*>(&(buffer_[0]));

              // Ensure input_source_selector's strings are null-terminated string even if corrupted data is sent.
              p->language[sizeof(p->language) - 1] = '\0';
              p->input_source_id[sizeof(p->input_source_id) - 1] = '\0';
              p->input_mode_id[sizeof(p->input_mode_id) - 1] = '\0';

              uint64_t time_stamp = p->time_stamp;
              boost::optional<std::string> language(std::string(p->language));
              boost::optional<std::string> input_source_id(std::string(p->input_source_id));
              boost::optional<std::string> input_mode_id(std::string(p->input_mode_id));
              if (language && language->empty()) {
                language = boost::none;
              }
              if (input_source_id && input_source_id->empty()) {
                input_source_id = boost::none;
              }
              if (input_mode_id && input_mode_id->empty()) {
                input_mode_id = boost::none;
              }

              input_source_selector input_source_selector(language,
                                                          input_source_id,
                                                          input_mode_id);

              dispatch_async(dispatch_get_main_queue(), ^{
                if (last_select_input_source_time_stamp_ == time_stamp) {
                  return;
                }
                if (input_source_manager_.select(input_source_selector)) {
                  last_select_input_source_time_stamp_ = time_stamp;
                }
              });
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

  input_source_manager input_source_manager_;
  uint64_t last_select_input_source_time_stamp_;
};
} // namespace krbn
