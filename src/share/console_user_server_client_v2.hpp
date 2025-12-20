#pragma once

// `krbn::console_user_server_client_v2` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <glob/glob.hpp>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/frontmost_application_monitor.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client_v2 final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;
  nod::signal<void(void)> next_heartbeat_deadline_exceeded;

  // Methods

  console_user_server_client_v2(const console_user_server_client_v2&) = delete;

  console_user_server_client_v2(std::optional<std::string> client_socket_directory_name)
      : dispatcher_client(),
        client_socket_directory_name_(client_socket_directory_name) {
  }

  virtual ~console_user_server_client_v2(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_server_client_v2 is already started.");
        return;
      }

      prepare_console_user_server_client_socket_directory();

      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               find_console_user_server_socket_file_path(),
                                                               console_user_server_client_socket_file_path(),
                                                               constants::local_datagram_buffer_size);
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));
      client_->set_server_socket_file_path_resolver([this] {
        return find_console_user_server_socket_file_path();
      });

      client_->connected.connect([this](auto&& peer_pid) {
        logger::get_logger()->info("console_user_server_client_v2 is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client_v2 connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });

        // connect_failed will be triggered if console_user_server_client_socket_directory does not exist
        // due to the parent directory (system_user_directory) is not ready.
        // For this case, we have to create console_user_server_client_socket_directory each time.
        prepare_console_user_server_client_socket_directory();
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("console_user_server_client_v2 is closed.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client_v2 error: {0}", error_code.message());
      });

      client_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          auto ot = json.at("operation_type").get<operation_type>();
          switch (ot) {
            case operation_type::shared_secret: {
              shared_secret_ = json.at("shared_secret").get<std::vector<uint8_t>>();
              break;
            }

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

      client_->next_heartbeat_deadline_exceeded.connect([this](auto&& sender_endpoint) {
        logger::get_logger()->info("console_user_server_client_v2 next heartbeat deadline exceeded");

        enqueue_to_dispatcher([this] {
          next_heartbeat_deadline_exceeded();
        });
      });

      client_->async_start();

      logger::get_logger()->info("console_user_server_client_v2 is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  // core_service -> console_user_server
  void async_handshake(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::handshake},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  // core_service -> console_user_server
  void async_shell_command_execution(const std::string& shell_command) const {
    enqueue_to_dispatcher([this, shell_command] {
      nlohmann::json json{
          {"operation_type", operation_type::shell_command_execution},
          {"shared_secret", shared_secret_},
          {"shell_command", shell_command},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  // core_service -> console_user_server
  void async_select_input_source(std::shared_ptr<std::vector<pqrs::osx::input_source_selector::specifier>> input_source_specifiers) {
    enqueue_to_dispatcher([this, input_source_specifiers] {
      if (input_source_specifiers) {
        nlohmann::json json{
            {"operation_type", operation_type::select_input_source},
            {"shared_secret", shared_secret_},
            {"input_source_specifiers", *input_source_specifiers},
        };

        if (client_) {
          client_->async_send(nlohmann::json::to_msgpack(json));
        }
      }
    });
  }

  // core_service -> console_user_server
  void async_software_function(const software_function& software_function) const {
    enqueue_to_dispatcher([this, software_function] {
      nlohmann::json json{
          {"operation_type", operation_type::software_function},
          {"shared_secret", shared_secret_},
          {"software_function", software_function},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  // event_viewer -> console_user_server
  void async_get_frontmost_application_history(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_frontmost_application_history},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

private:
  std::filesystem::path find_console_user_server_socket_file_path(void) const {
    return filesystem_utility::find_socket_file_path(
        constants::get_console_user_server_socket_directory_path(geteuid()));
  }

  std::optional<std::filesystem::path> console_user_server_client_socket_directory_path(void) const {
    if (client_socket_directory_name_ != std::nullopt &&
        client_socket_directory_name_ != "") {
      return constants::get_system_user_directory(geteuid()) / *client_socket_directory_name_;
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
    }
  }

  void stop(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;

    logger::get_logger()->info("console_user_server_client_v2 is stopped.");
  }

  std::optional<std::string> client_socket_directory_name_;
  std::unique_ptr<pqrs::local_datagram::client> client_;
  std::vector<uint8_t> shared_secret_;
};
} // namespace krbn
