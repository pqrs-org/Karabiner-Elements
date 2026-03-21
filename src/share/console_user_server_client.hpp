#pragma once

// `krbn::console_user_server_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include "shared_secret_authentication.hpp"
#include "types.hpp"
#include <glob/glob.hpp>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(uid_t uid,
                             std::optional<std::string> client_socket_directory_name)
      : dispatcher_client(),
        uid_(uid),
        client_socket_directory_name_(client_socket_directory_name) {
  }

  virtual ~console_user_server_client(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_server_client is already started.");
        return;
      }

      prepare_console_user_server_client_socket_directory();

      auto client_socket_file_path = console_user_server_client_socket_file_path();

      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               find_console_user_server_socket_file_path(),
                                                               client_socket_file_path,
                                                               constants::local_datagram_buffer_size);
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));
      client_->set_server_socket_file_path_resolver([this] {
        return find_console_user_server_socket_file_path();
      });

      client_->connected.connect([this, client_socket_file_path](auto&& peer_pid) {
        logger::get_logger()->info("console_user_server_client is connected.");

        if (uid_ != geteuid()) {
          if (auto p = client_socket_file_path) {
            chown(p->c_str(), uid_, 0);
          }
        }

        enqueue_to_dispatcher([this] {
          if (client_) {
            shared_secret_authentication_client_.connected(*client_);
          }
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });

        // connect_failed will be triggered if console_user_server_client_socket_directory does not exist
        // due to the parent directory (system_user_directory) is not ready.
        // For this case, we have to create console_user_server_client_socket_directory each time.
        prepare_console_user_server_client_socket_directory();
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("console_user_server_client is closed.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client error: {0}", error_code.message());
      });

      client_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
        if (buffer->empty()) {
          return;
        }

        try {
          if (client_ &&
              shared_secret_authentication_client_.handle_shared_secret_payload(*buffer,
                                                                                *client_,
                                                                                [this] {
                                                                                  connected();
                                                                                })) {
            return;
          }

          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          auto ot = json.at("operation_type").get<operation_type>();
          switch (ot) {
            case operation_type::user_core_configuration_file_path:
            case operation_type::frontmost_application_history:
              enqueue_to_dispatcher([this, ot, json] {
                received(ot, json);
              });
              break;

            default:
              break;
          }
        } catch (std::exception& e) {
          logger::get_logger()->error("received data is corrupted");
        }
      });

      client_->async_start();

      logger::get_logger()->info("console_user_server_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_get_user_core_configuration_file_path(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_user_core_configuration_file_path},
      };

      async_send_message(std::move(json));
    });
  }

  void async_check_for_updates_on_startup(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::check_for_updates_on_startup},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_menu_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_menu_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_menu_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::unregister_menu_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_multitouch_extension_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_multitouch_extension_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_multitouch_extension_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::unregister_multitouch_extension_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_notification_window_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_notification_window_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_notification_window_agent(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::unregister_notification_window_agent},
      };

      async_send_message(std::move(json));
    });
  }

  // core_service (daemon) -> console_user_server
  void async_frontmost_application_changed(const application& application) const {
    enqueue_to_dispatcher([this, application] {
      nlohmann::json json{
          {"operation_type", operation_type::frontmost_application_changed},
          {"frontmost_application", application},
      };

      async_send_message(std::move(json));
    });
  }

  // core_service -> console_user_server
  void async_shell_command_execution(const std::string& shell_command) const {
    enqueue_to_dispatcher([this, shell_command] {
      nlohmann::json json{
          {"operation_type", operation_type::shell_command_execution},
          {"shell_command", shell_command},
      };

      async_send_message(std::move(json));
    });
  }

  // core_service -> console_user_server
  void async_send_user_command(const nlohmann::json& user_command) const {
    enqueue_to_dispatcher([this, user_command] {
      nlohmann::json json{
          {"operation_type", operation_type::send_user_command},
          {"user_command", user_command},
      };

      async_send_message(std::move(json));
    });
  }

  // core_service -> console_user_server
  void async_select_input_source(std::shared_ptr<std::vector<pqrs::osx::input_source_selector::specifier>> input_source_specifiers) {
    enqueue_to_dispatcher([this, input_source_specifiers] {
      if (input_source_specifiers) {
        nlohmann::json json{
            {"operation_type", operation_type::select_input_source},
            {"input_source_specifiers", *input_source_specifiers},
        };

        async_send_message(std::move(json));
      }
    });
  }

  // core_service -> console_user_server
  void async_software_function(const software_function& software_function) const {
    enqueue_to_dispatcher([this, software_function] {
      nlohmann::json json{
          {"operation_type", operation_type::software_function},
          {"software_function", software_function},
      };

      async_send_message(std::move(json));
    });
  }

  // event_viewer -> console_user_server
  void async_get_frontmost_application_history(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_frontmost_application_history},
      };

      async_send_message(std::move(json));
    });
  }

private:
  void async_send_message(nlohmann::json&& json,
                          std::function<void(void)> processed = nullptr) const {
    if (!client_) {
      return;
    }

    shared_secret_authentication_client_.async_send_message(*client_,
                                                            std::move(json),
                                                            processed);
  }

  std::filesystem::path find_console_user_server_socket_file_path(void) const {
    return filesystem_utility::find_socket_file_path(
        constants::get_console_user_server_socket_directory_path(uid_));
  }

  std::optional<std::filesystem::path> console_user_server_client_socket_directory_path(void) const {
    if (client_socket_directory_name_ != std::nullopt &&
        client_socket_directory_name_ != "") {
      return constants::get_system_user_directory(uid_) / *client_socket_directory_name_;
    }

    return std::nullopt;
  }

  std::optional<std::filesystem::path> console_user_server_client_socket_file_path(void) const {
    if (auto d = console_user_server_client_socket_directory_path()) {
      return *d / filesystem_utility::make_socket_file_basename();
    }

    return std::nullopt;
  }

  void prepare_console_user_server_client_socket_directory(void) {
    if (auto d = console_user_server_client_socket_directory_path()) {

      //
      // Remove old socket files.
      //

      std::error_code ec;
      std::filesystem::remove_all(*d, ec);

      //
      // Create directory.
      //

      std::filesystem::create_directory(*d, ec);

      if (uid_ != geteuid()) {
        chown(d->c_str(), uid_, 0);
      }
    }
  }

  void stop(void) {
    if (!client_) {
      return;
    }

    shared_secret_authentication_client_.reset();
    client_ = nullptr;

    logger::get_logger()->info("console_user_server_client is stopped.");
  }

  uid_t uid_;
  std::optional<std::string> client_socket_directory_name_;

  std::unique_ptr<pqrs::local_datagram::client> client_;
  mutable shared_secret_authentication::client shared_secret_authentication_client_;
};
} // namespace krbn
