#pragma once

// `krbn::core_service_client` can be used safely in a multi-threaded environment.

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
class core_service_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  core_service_client(const core_service_client&) = delete;

  core_service_client()
      : dispatcher_client() {
  }

  ~core_service_client() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("core_service_client is already started.");
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
          constants::get_karabiner_core_service_socket_file_path(),
          options,
          [](const auto& peer_credentials) {
            auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
            if (!result) {
              logger::get_logger()->warn("core_service_client: peer is not code-signed with same Team ID");
            }
            return result;
          });

      client_->connected.connect([this](auto&&) {
        logger::get_logger()->info("core_service_client is connected.");

        enqueue_to_dispatcher([this] {
          ++connection_generation_;
          connection_ready_ = true;

          flush_pending_messages();
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->error("core_service_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          make_connection_not_ready();

          connect_failed(error_code);
        });
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("core_service_client is closed.");

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();

          closed();
        });
      });

      client_->error_occurred.connect([this](auto&& error_code) {
        logger::get_logger()->error("core_service_client error: {0}", error_code.message());

        enqueue_to_dispatcher([this] {
          make_connection_not_ready();
        });
      });

      client_->peer_verification_failed.connect([this](auto&&) {
        logger::get_logger()->error("core_service_client peer_verification_failed");

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
            logger::get_logger()->error("core_service_client received data is corrupted");
          }
        });
      });

      client_->async_start();

      logger::get_logger()->info("core_service_client is started.");
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_system_preferences_updated(std::shared_ptr<pqrs::osx::system_preferences::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::system_preferences_updated},
            {"system_preferences_properties", *properties},
        };

        async_send_message(std::move(json));
      }
    });
  }

  void async_core_service_bundle_permission_check_result(bool iohid_listen_event_allowed,
                                                         bool accessibility_process_trusted) const {
    enqueue_to_dispatcher([this, iohid_listen_event_allowed, accessibility_process_trusted] {
      nlohmann::json json{
          {"operation_type", operation_type::core_service_bundle_permission_check_result},
          {"iohid_listen_event_allowed", iohid_listen_event_allowed},
          {"accessibility_process_trusted", accessibility_process_trusted},
      };

      async_send_message(std::move(json));
    });
  }

  void async_frontmost_application_changed(const application& application) const {
    enqueue_to_dispatcher([this, application] {
      nlohmann::json json{
          {"operation_type", operation_type::frontmost_application_changed},
          {"frontmost_application", application},
      };

      async_send_message(std::move(json));
    });
  }

  void async_focused_ui_element_changed(const focused_ui_element& focused_ui_element) const {
    enqueue_to_dispatcher([this, focused_ui_element] {
      nlohmann::json json{
          {"operation_type", operation_type::focused_ui_element_changed},
          {"focused_ui_element", focused_ui_element},
      };

      async_send_message(std::move(json));
    });
  }

  void async_input_source_changed(std::shared_ptr<pqrs::osx::input_source::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::input_source_changed},
            {"input_source_properties", *properties},
        };

        async_send_message(std::move(json));
      }
    });
  }

  void async_temporarily_ignore_all_devices(bool value) const {
    enqueue_to_dispatcher([this, value] {
      nlohmann::json json{
          {"operation_type", operation_type::temporarily_ignore_all_devices},
          {"value", value},
      };

      async_send_message(std::move(json));
    });
  }

  void async_get_manipulator_environment() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_manipulator_environment},
      };

      async_send_message(std::move(json));
    });
  }

  void async_get_connected_devices() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_connected_devices},
      };

      async_send_message(std::move(json));
    });
  }

  void async_get_notification_message() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_notification_message},
      };

      async_send_message(std::move(json));
    });
  }

  void async_get_system_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_system_variables},
      };

      async_send_message(std::move(json));
    });
  }

  void async_get_multitouch_extension_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_multitouch_extension_variables},
      };

      async_send_message(std::move(json));
    });
  }

  void async_connect_multitouch_extension() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::connect_multitouch_extension},
      };

      async_send_message(std::move(json));
    });
  }

  void async_set_app_icon(int number) const {
    enqueue_to_dispatcher([this, number] {
      nlohmann::json json{
          {"operation_type", operation_type::set_app_icon},
          {"number", number},
      };

      async_send_message(std::move(json));
    });
  }

  /**
   * @brief Set variables
   *
   * @param variables nlohmann::json::object which type is {[key: string]: number|boolean|string}.
   * @param processed A callback which is called when the request is processed.
   *                  (When data is sent to core_service or error occurred)
   */
  void async_set_variables(const nlohmann::json& variables,
                           std::function<void()> processed = nullptr) const {
    enqueue_to_dispatcher([this, variables, processed] {
      nlohmann::json json{
          {"operation_type", operation_type::set_variables},
          {"variables", variables},
      };

      async_send_message(std::move(json),
                         processed);
    });
  }

  void async_clear_user_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::clear_user_variables},
      };

      async_send_message(std::move(json));
    });
  }

private:
  void async_send_message(nlohmann::json&& json,
                          std::function<void()> processed = nullptr) const {
    if (!client_) {
      return;
    }

    if (!connection_ready_) {
      pending_messages_.push_back(pending_message{
          .json = std::move(json),
          .processed = processed,
      });
      return;
    }

    auto connection_generation = connection_generation_;
    client_->async_request(
        nlohmann::json::to_msgpack(json),
        [this, connection_generation, processed](auto&& error_code, auto&& buffer) {
          enqueue_to_dispatcher([this, connection_generation, error_code, buffer, processed] {
            if (connection_generation_ != connection_generation) {
              if (processed) {
                processed();
              }
              return;
            }

            if (error_code) {
              logger::get_logger()->error("core_service_client request failed: {0}", error_code.message());

              make_connection_not_ready();

              if (client_) {
                client_->async_invalidate_connection();
              }
            }

            if (buffer &&
                !buffer->empty()) {
              handle_response(buffer);
            }

            if (processed) {
              processed();
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
      logger::get_logger()->error("core_service_client received data is corrupted");
    }
  }

  struct pending_message final {
    nlohmann::json json;
    std::function<void()> processed;
  };

  void flush_pending_messages() const {
    auto pending_messages = std::exchange(pending_messages_, {});

    for (auto& message : pending_messages) {
      async_send_message(std::move(message.json),
                         message.processed);
    }
  }

  void stop() {
    if (!client_) {
      return;
    }

    client_ = nullptr;
    pending_messages_.clear();
    make_connection_not_ready();

    logger::get_logger()->info("core_service_client is stopped.");
  }

  void make_connection_not_ready() const {
    connection_ready_ = false;
    ++connection_generation_;
  }

  std::unique_ptr<pqrs::unix_domain_stream::client> client_;
  mutable std::vector<pending_message> pending_messages_;
  mutable bool connection_ready_ = false;
  mutable uint64_t connection_generation_ = 0;
};
} // namespace krbn
