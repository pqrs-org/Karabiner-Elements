#pragma once

// `krbn::console_user_server::receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "send_user_command_handler.hpp"
#include "services_utility.hpp"
#include "shell_command_handler.hpp"
#include "software_function_handler.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>

namespace krbn {
namespace console_user_server {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<software_function_handler> weak_software_function_handler)
      : dispatcher_client(),
        weak_software_function_handler_(weak_software_function_handler),
        input_source_selector_(std::make_unique<pqrs::osx::input_source_selector::selector>(weak_dispatcher_)),
        shell_command_handler_(std::make_unique<shell_command_handler>()),
        send_user_command_handler_(std::make_unique<send_user_command_handler>()) {
    // Remove old files and prepare a socket directory.
    prepare_console_user_server_socket_directory();

    verified_peer_manager_ = std::make_unique<pqrs::local_datagram::extra::peer_manager>(
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
        auto ot = json.at("operation_type").get<operation_type>();
        switch (ot) {
          case operation_type::handshake:
            if (verified_peer_manager_) {
              std::vector<uint8_t> shared_secret(32);
              arc4random_buf(shared_secret.data(), shared_secret.size());

              verified_peer_manager_->insert_shared_secret(sender_endpoint->path(),
                                                           shared_secret);

              nlohmann::json json{
                  {"operation_type", operation_type::shared_secret},
                  {"shared_secret", shared_secret},
              };
              verified_peer_manager_->async_send(sender_endpoint->path(),
                                                 nlohmann::json::to_msgpack(json));
            }
            break;

          case operation_type::get_user_core_configuration_file_path:
            if (verified_peer_manager_) {
              nlohmann::json json{
                  {"operation_type", operation_type::user_core_configuration_file_path},
                  {"user_core_configuration_file_path", constants::get_user_core_configuration_file_path()},
              };
              verified_peer_manager_->async_send(sender_endpoint->path(),
                                                 nlohmann::json::to_msgpack(json));
            }
            break;

          case operation_type::check_for_updates_on_startup:
          case operation_type::register_menu_agent:
          case operation_type::unregister_menu_agent:
          case operation_type::register_multitouch_extension_agent:
          case operation_type::unregister_multitouch_extension_agent:
          case operation_type::register_notification_window_agent:
          case operation_type::unregister_notification_window_agent:
          case operation_type::select_input_source:
          case operation_type::shell_command_execution:
          case operation_type::send_user_command:
          case operation_type::software_function:
            if (verified_peer_manager_) {
              if (verified_peer_manager_->verify_shared_secret(sender_endpoint->path(),
                                                               json.at("shared_secret").get<std::vector<uint8_t>>())) {
                switch (ot) {
                  case operation_type::check_for_updates_on_startup: {
                    static bool checked = false;
                    if (!checked) {
                      checked = true;

                      logger::get_logger()->info("operation_type::check_for_updates_on_startup was received; waiting 30 seconds before checking for updates.");

                      // Note:
                      //
                      // During the updates, Karabiner-Updater.app and console_user_server binaries are overwritten asynchronous.
                      // And console_user_server will be restarted via version check.
                      // If console_user_server is restarted before Karabiner-Updater.app overwritten,
                      // checking for updates runs with the old version of Karabiner-Updater.app,
                      // and the update dialog is shown again even though the update was just completed.
                      //
                      // Wait before checking for updates to avoid it.
                      enqueue_to_dispatcher(
                          [] {
                            logger::get_logger()->info("Check for updates...");
                            update_utility::check_for_updates_in_background();
                          },
                          when_now() + std::chrono::seconds(30));
                    }

                    break;
                  }

                  case operation_type::register_menu_agent:
                    services_utility::register_menu_agent();
                    break;

                  case operation_type::unregister_menu_agent:
                    services_utility::unregister_menu_agent();
                    break;

                  case operation_type::register_multitouch_extension_agent:
                    services_utility::register_multitouch_extension_agent();
                    break;

                  case operation_type::unregister_multitouch_extension_agent:
                    services_utility::unregister_multitouch_extension_agent();
                    break;

                  case operation_type::register_notification_window_agent:
                    services_utility::register_notification_window_agent();
                    break;

                  case operation_type::unregister_notification_window_agent:
                    services_utility::unregister_notification_window_agent();
                    break;

                  case operation_type::select_input_source:
                    if (input_source_selector_) {
                      using specifiers_t = std::vector<pqrs::osx::input_source_selector::specifier>;
                      auto specifiers = json.at("input_source_specifiers").get<specifiers_t>();
                      input_source_selector_->async_select(std::make_shared<specifiers_t>(specifiers));
                    }
                    break;

                  case operation_type::shell_command_execution:
                    if (shell_command_handler_) {
                      auto shell_command = json.at("shell_command").get<std::string>();
                      shell_command_handler_->run(shell_command);
                    }
                    break;

                  case operation_type::send_user_command:
                    if (send_user_command_handler_) {
                      auto user_command = json.at("user_command").get<nlohmann::json>();
                      send_user_command_handler_->run(user_command);
                    }
                    break;

                  case operation_type::software_function:
                    if (auto h = weak_software_function_handler_.lock()) {
                      h->execute_software_function(json.at("software_function").get<software_function>());
                    }
                    break;

                  default:
                    break;
                }
              } else {
                logger::get_logger()->error("operation_type::{0} with invalid shared secret",
                                            json.at("operation_type").get<std::string>());
              }
            }
            break;

          case operation_type::get_frontmost_application_history:
            if (auto h = weak_software_function_handler_.lock()) {
              h->async_invoke_with_frontmost_application_history(
                  [this, sender_endpoint](auto&& frontmost_application_history) {
                    if (verified_peer_manager_) {
                      nlohmann::json json{
                          {"operation_type", operation_type::frontmost_application_history},
                          {"frontmost_application_history", frontmost_application_history}};
                      verified_peer_manager_->async_send(sender_endpoint->path(),
                                                         nlohmann::json::to_msgpack(json));
                    }
                  });
            }
            break;

          default:
            break;
        }
      } catch (std::exception& e) {
        logger::get_logger()->error("received data is corrupted");
      }
    });

    server_->async_start();

    logger::get_logger()->info("receiver is initialized");
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      input_source_selector_ = nullptr;
      shell_command_handler_ = nullptr;
      send_user_command_handler_ = nullptr;

      server_ = nullptr;
      verified_peer_manager_ = nullptr;
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  std::filesystem::path console_user_server_socket_file_path(void) const {
    return constants::get_console_user_server_socket_directory_path(geteuid()) / filesystem_utility::make_socket_file_basename();
  }

  void prepare_console_user_server_socket_directory(void) const {
    auto directory_path = console_user_server_socket_file_path().parent_path();

    std::error_code ec;
    std::filesystem::remove_all(directory_path, ec);
    std::filesystem::create_directory(directory_path, ec);
    chmod(directory_path.c_str(), 0755);
  }

  std::weak_ptr<software_function_handler> weak_software_function_handler_;
  std::unique_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
  std::unique_ptr<shell_command_handler> shell_command_handler_;
  std::unique_ptr<send_user_command_handler> send_user_command_handler_;

  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> verified_peer_manager_;
  std::unique_ptr<pqrs::local_datagram::server> server_;
};
} // namespace console_user_server
} // namespace krbn
