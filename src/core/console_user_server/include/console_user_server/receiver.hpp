#pragma once

// `krbn::console_user_server::receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "software_function_handler.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>

namespace krbn {
namespace console_user_server {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<software_function_handler> weak_software_function_handler)
      : dispatcher_client(),
        weak_software_function_handler_(weak_software_function_handler) {
    // Remove old files and prepare a socket directory.
    prepare_console_user_server_socket_directory();

    event_viewer_get_frontmost_application_history_peer_manager_ = std::make_unique<pqrs::local_datagram::extra::peer_manager>(
        weak_dispatcher_,
        constants::local_datagram_buffer_size,
        [](auto&& peer_pid,
           auto&& peer_socket_file_path) {
          // Verify the peer's Team ID before sending manipulator environment information.
          if (get_shared_codesign_manager()->same_team_id(peer_pid)) {
            logger::get_logger()->info("verified peer connected");
            return true;
          } else {
            logger::get_logger()->warn("peer is not code-signed with same Team ID");
            return false;
          }
        });

    auto socket_file_path = console_user_server_socket_file_path();

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             constants::local_datagram_buffer_size);
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([socket_file_path] {
      logger::get_logger()->info("receiver: bound");
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("receiver: bind_failed");

      // If the socket directory is deleted for any reason,
      // bind_failed will be triggered, so recreate the directory each time.
      prepare_console_user_server_socket_directory();
    });

    server_->closed.connect([] {
      logger::get_logger()->info("receiver: closed");
    });

    server_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
      if (buffer->empty()) {
        return;
      }

      try {
        nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
        switch (json.at("operation_type").get<operation_type>()) {
          case operation_type::get_frontmost_application_history:
            if (auto h = weak_software_function_handler_.lock()) {
              h->async_invoke_with_frontmost_application_history(
                  [this, sender_endpoint](auto&& frontmost_application_history) {
                    if (event_viewer_get_frontmost_application_history_peer_manager_) {
                      nlohmann::json json{
                          {"operation_type", operation_type::frontmost_application_history},
                          {"frontmost_application_history", frontmost_application_history}};
                      event_viewer_get_frontmost_application_history_peer_manager_->async_send(sender_endpoint->path(),
                                                                                               nlohmann::json::to_msgpack(json));
                    }
                  });
            }
            break;

          default:
            break;
        }
        return;
      } catch (std::exception& e) {
        logger::get_logger()->error("received data is corrupted");
      }
    });

    server_->async_start();

    logger::get_logger()->info("receiver is initialized");
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
      event_viewer_get_frontmost_application_history_peer_manager_ = nullptr;
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  std::filesystem::path console_user_server_socket_directory(void) const {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name console_user_server -> con_usr_srv.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/con_usr_srv/1880fdaa305fe8f0.sock`

    return constants::get_system_user_directory(geteuid()) / std::filesystem::path("con_usr_srv");
  }

  std::filesystem::path console_user_server_socket_file_path(void) const {
    return console_user_server_socket_directory() / filesystem_utility::make_socket_file_basename();
  }

  void prepare_console_user_server_socket_directory(void) const {
    auto directory_path = console_user_server_socket_directory();
    std::error_code ec;
    std::filesystem::remove_all(directory_path, ec);
    std::filesystem::create_directory(directory_path, ec);
    chmod(directory_path.c_str(), 0755);
  }

  std::weak_ptr<software_function_handler> weak_software_function_handler_;

  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> event_viewer_get_frontmost_application_history_peer_manager_;
  std::unique_ptr<pqrs::local_datagram::server> server_;
};
} // namespace console_user_server
} // namespace krbn
