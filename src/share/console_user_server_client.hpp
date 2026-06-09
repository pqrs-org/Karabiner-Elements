#pragma once

// `krbn::console_user_server_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <unistd.h>
#include <utility>
#include <vector>

namespace krbn {
class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  explicit console_user_server_client(uid_t uid)
      : dispatcher_client(),
        uid_(uid) {
  }

  virtual ~console_user_server_client() {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("console_user_server_client is already started.");
        return;
      }

      auto options = pqrs::unix_domain_stream::options(
          pqrs::unix_domain_stream::options::initialization_parameters{
              .max_message_size = constants::unix_domain_stream_max_message_size,
              .reconnect_interval = std::chrono::milliseconds(1000),
              .server_check_interval = std::chrono::milliseconds(3000),
          });

      client_ = std::make_unique<pqrs::unix_domain_stream::client>(
          weak_dispatcher_,
          constants::get_console_user_server_socket_file_path(uid_),
          options,
          [](const auto& peer_credentials) {
            auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
            if (!result) {
              logger::get_logger()->warn("console_user_server_client: peer is not code-signed with same Team ID");
            }
            return result;
          });

      client_->connected.connect([this](auto&&) {
        logger::get_logger()->info("console_user_server_client is connected.");

        enqueue_to_dispatcher([this] {
          ++connection_generation_;
          connection_ready_ = true;

          flush_pending_messages();
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          make_connection_not_ready();

          connect_failed(error_code);
        });
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("console_user_server_client is closed.");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();

          closed();
        });
      });

      client_->error_occurred.connect([this](auto&& error_code) {
        logger::get_logger()->error("console_user_server_client error: {0}", error_code.message());

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->peer_verification_failed.connect([this](auto&&) {
        logger::get_logger()->error("console_user_server_client peer_verification_failed");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->received.connect([this](auto&& buffer) {
        enqueue_to_dispatcher([this, buffer] {
          if (buffer->empty()) {
            return;
          }

          try {
            auto json = nlohmann::json::from_msgpack(*buffer);
            auto ot = json.at("operation_type").template get<operation_type>();

            received(ot,
                     json);
          } catch (std::exception& e) {
            logger::get_logger()->error("received data is corrupted");
          }
        });
      });

      client_->async_start();

      logger::get_logger()->info("console_user_server_client is started.");
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_get_user_core_configuration_file_path() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_user_core_configuration_file_path},
      };

      async_send_message(std::move(json));
    });
  }

  void async_check_for_updates_on_startup() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::check_for_updates_on_startup},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_menu_agent() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_menu_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_menu_agent() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::unregister_menu_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_multitouch_extension_agent() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_multitouch_extension_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_multitouch_extension_agent() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::unregister_multitouch_extension_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_register_notification_window_agent() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::register_notification_window_agent},
      };

      async_send_message(std::move(json));
    });
  }

  void async_unregister_notification_window_agent() const {
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

  // core_service (daemon) -> console_user_server
  void async_focused_ui_element_changed(const focused_ui_element& focused_ui_element) const {
    enqueue_to_dispatcher([this, focused_ui_element] {
      nlohmann::json json{
          {"operation_type", operation_type::focused_ui_element_changed},
          {"focused_ui_element", focused_ui_element},
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

  // settings_window -> console_user_server
  void async_get_settings_window_guidance() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_settings_window_guidance},
      };

      async_send_message(std::move(json));
    });
  }

  // core_service (daemon) -> console_user_server
  void async_core_service_daemon_state(const nlohmann::json& core_service_daemon_state) const {
    enqueue_to_dispatcher([this, core_service_daemon_state] {
      nlohmann::json json{
          {"operation_type", operation_type::core_service_daemon_state},
          {"core_service_daemon_state", core_service_daemon_state},
      };

      async_send_message(std::move(json));
    });
  }

  // event_viewer -> console_user_server
  void async_get_frontmost_application_history() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_frontmost_application_history},
      };

      async_send_message(std::move(json));
    });
  }

private:
  void async_send_message(nlohmann::json&& json) const {
    if (!client_) {
      return;
    }

    if (!connection_ready_) {
      pending_messages_.push_back(std::move(json));
      return;
    }

    auto connection_generation = connection_generation_;
    client_->async_request(
        nlohmann::json::to_msgpack(json),
        [this, connection_generation](auto&& error_code, auto&& buffer) {
          enqueue_to_dispatcher([this, connection_generation, error_code, buffer] {
            if (connection_generation_ != connection_generation) {
              return;
            }

            if (error_code) {
              logger::get_logger()->error("console_user_server_client request failed: {0}", error_code.message());

              make_connection_not_ready();

              if (client_) {
                client_->async_invalidate_connection();
              }
            }

            if (buffer &&
                !buffer->empty()) {
              handle_response(buffer);
            }
          });
        });
  }

  void handle_response(pqrs::not_null_shared_ptr_t<std::vector<uint8_t>> buffer) const {
    try {
      auto json = nlohmann::json::from_msgpack(*buffer);
      auto ot = json.at("operation_type").template get<operation_type>();
      if (ot == operation_type::none) {
        return;
      }

      received(ot,
               json);
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
    }
  }

  void flush_pending_messages() const {
    auto pending_messages = std::exchange(pending_messages_, {});

    for (auto& message : pending_messages) {
      async_send_message(std::move(message));
    }
  }

  void stop() {
    if (!client_) {
      return;
    }

    client_ = nullptr;
    pending_messages_.clear();
    make_connection_not_ready();

    logger::get_logger()->info("console_user_server_client is stopped.");
  }

  void make_connection_not_ready() const {
    connection_ready_ = false;
    ++connection_generation_;
  }

  uid_t uid_;

  std::unique_ptr<pqrs::unix_domain_stream::client> client_;
  mutable std::vector<nlohmann::json> pending_messages_;
  mutable bool connection_ready_ = false;
  mutable uint64_t connection_generation_ = 0;
};
} // namespace krbn
