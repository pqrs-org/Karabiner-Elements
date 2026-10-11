#pragma once

// `krbn::core_service_daemon_client` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <filesystem>
#include <functional>
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <pqrs/osx/xpc.hpp>
#include <unistd.h>
#include <utility>
#include <vector>

namespace krbn {
namespace xpc = pqrs::osx::xpc;

class core_service_daemon_client final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> connected;
  nod::signal<void(const std::error_code&)> connect_failed;
  nod::signal<void()> closed;
  nod::signal<void(operation_type, const nlohmann::json&)> received;

  // Methods

  core_service_daemon_client(const core_service_daemon_client&) = delete;

  core_service_daemon_client() {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~core_service_daemon_client() override {
    unregister_callbacks_and_detach();
  }

  // Stop callbacks and dispatcher activity at the owner's lifecycle boundary.
  //
  // Do not rely only on the destructor for this cleanup. A caller can hold a
  // temporary shared_ptr after the owner releases its reference, which would
  // otherwise leave callbacks that capture the owner alive.
  void unregister_callbacks_and_detach() {
    std::call_once(unregister_callbacks_and_detach_once_, [this] {
      detach_from_dispatcher([this] {
        connected.disconnect_all_slots();
        connect_failed.disconnect_all_slots();
        closed.disconnect_all_slots();
        received.disconnect_all_slots();

        stop();
      });
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("core_service_daemon_client is already started.");
        return;
      }

      client_ = std::make_unique<xpc::client>(
          weak_dispatcher_,
          xpc::client_options({
              .common_parameters = {
                  .signing_requirement = get_shared_codesign_manager()->xpc_signing_requirement({
                      "org.pqrs.Karabiner-Core-Service",
                  }),
                  .max_message_size = constants::unix_domain_stream_max_message_size,
              },
              .client_parameters = {
                  .service_name = constants::get_core_service_daemon_mach_service_name(),
                  .privileged = true,
              },
          }));

      client_->peer_ready.connect([this](auto peer_id) {
        peer_id_ = peer_id;
        logger::get_logger()->debug("core_service_daemon_client is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connection_failed.connect([this](const auto& error_code) {
        logger::get_logger()->debug("core_service_daemon_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->peer_interrupted.connect([this](auto peer_id) {
        if (peer_id_ == peer_id) {
          peer_id_.reset();
        }
        logger::get_logger()->debug("core_service_daemon_client is interrupted.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->peer_invalidated.connect([this](auto peer_id,
                                               const auto& error_code) {
        if (peer_id_ == peer_id) {
          peer_id_.reset();
        }
        logger::get_logger()->debug("core_service_daemon_client peer_invalidated: {0}",
                                    error_code.message());

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](const auto& error) {
        logger::get_logger()->debug("core_service_daemon_client error: {0}",
                                    error.message);
      });

      client_->message_received.connect([this](auto, auto&& buffer) {
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
            logger::get_logger()->error("core_service_daemon_client received data is corrupted");
          }
        });
      });

      client_->request_received.connect([this](auto, auto reply,
                                               auto&& buffer) {
        // Reply before handling the message, since some handlers run synchronous helper commands
        // such as service registration, which may exceed the request timeout and close the peer.
        // This response means the request has been accepted, not fully processed.
        if (client_) {
          client_->async_reply(
              reply,
              std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(nlohmann::json{
                  {"operation_type", operation_type::none},
              })));
        }

        if (!buffer->empty()) {
          handle_message(buffer);
        }
      });

      client_->async_start();

      logger::get_logger()->debug("core_service_daemon_client is started.");
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

        async_request(std::move(json));
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

      async_request(std::move(json));
    });
  }

  void async_frontmost_application_changed(const application& application) const {
    enqueue_to_dispatcher([this, application] {
      nlohmann::json json{
          {"operation_type", operation_type::frontmost_application_changed},
          {"frontmost_application", application},
      };

      async_request(std::move(json));
    });
  }

  void async_focused_ui_element_changed(const focused_ui_element& focused_ui_element) const {
    enqueue_to_dispatcher([this, focused_ui_element] {
      nlohmann::json json{
          {"operation_type", operation_type::focused_ui_element_changed},
          {"focused_ui_element", focused_ui_element},
      };

      async_request(std::move(json));
    });
  }

  void async_input_source_changed(std::shared_ptr<pqrs::osx::input_source::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::input_source_changed},
            {"input_source_properties", *properties},
        };

        async_request(std::move(json));
      }
    });
  }

  void async_start_device_grabber(const std::filesystem::path& user_core_configuration_file_path) const {
    enqueue_to_dispatcher([this, user_core_configuration_file_path] {
      nlohmann::json json{
          {"operation_type", operation_type::start_device_grabber},
          {"user_core_configuration_file_path", user_core_configuration_file_path},
      };

      async_request(std::move(json));
    });
  }

  void async_temporarily_ignore_device(std::optional<device_id> device_id) const {
    enqueue_to_dispatcher([this, device_id] {
      nlohmann::json json{
          {"operation_type", operation_type::temporarily_ignore_device},
          {"device_id", device_id ? nlohmann::json(type_safe::get(*device_id)) : nlohmann::json(nullptr)},
      };

      async_request(std::move(json));
    });
  }

  void async_get_manipulator_environment() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_manipulator_environment},
      };

      async_request(std::move(json));
    });
  }

  void async_observe_connected_devices() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::observe_connected_devices},
      };

      async_request(std::move(json));
    });
  }

  void async_observe_notification_message() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::observe_notification_message},
      };

      async_request(std::move(json));
    });
  }

  void async_get_system_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_system_variables},
      };

      async_request(std::move(json));
    });
  }

  void async_get_multitouch_extension_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_multitouch_extension_variables},
      };

      async_request(std::move(json));
    });
  }

  void async_connect_multitouch_extension() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::connect_multitouch_extension},
      };

      async_request(std::move(json));
    });
  }

  void async_set_app_icon(int number) const {
    enqueue_to_dispatcher([this, number] {
      nlohmann::json json{
          {"operation_type", operation_type::set_app_icon},
          {"number", number},
      };

      async_request(std::move(json));
    });
  }

  void async_set_variables(const nlohmann::json& variables) const {
    async_set_variables_with_completion_handler(variables,
                                                nullptr);
  }

  void async_set_variables_with_completion_handler(
      const nlohmann::json& variables,
      std::function<void(const std::error_code&)> completion_handler) const {
    enqueue_to_dispatcher([this, variables, completion_handler = std::move(completion_handler)] {
      nlohmann::json json{
          {"operation_type", operation_type::set_variables},
          {"variables", variables},
      };

      async_request(std::move(json),
                    std::move(completion_handler));
    });
  }

  void async_clear_user_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::clear_user_variables},
      };

      async_request(std::move(json));
    });
  }

private:
  using request_completion_handler = std::function<void(const std::error_code&)>;

  void async_request(nlohmann::json&& json,
                     request_completion_handler completion_handler = nullptr) const {
    if (!client_) {
      if (completion_handler) {
        completion_handler(xpc::make_error_code(xpc::errc::not_ready));
      }
      return;
    }

    if (!peer_id_) {
      if (completion_handler) {
        completion_handler(xpc::make_error_code(xpc::errc::not_ready));
      }
      return;
    }

    // The listener checks this UID against the active console account.
    json["uid"] = getuid();

    client_->async_request(
        xpc::client::async_request_parameters{
            .id = *peer_id_,
            .data = std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)),
            .completion = [this, completion_handler = std::move(completion_handler)](auto result) {
              if (!result) {
                logger::get_logger()->debug("core_service_daemon_client request failed: {0}",
                                            result.error().message());
              }

              if (result &&
                  !(*result)->empty()) {
                handle_message(*result);
              }

              if (completion_handler) {
                completion_handler(result
                                       ? std::error_code{}
                                       : result.error());
              }
            },
        });
  }

  void handle_message(pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>> buffer) const {
    try {
      auto json = nlohmann::json::from_msgpack(*buffer);
      auto ot = json.at("operation_type").template get<operation_type>();
      if (ot == operation_type::none) {
        return;
      }

      received(ot,
               json);
    } catch (std::exception& e) {
      logger::get_logger()->error("core_service_daemon_client received data is corrupted");
    }
  }

  void stop() {
    if (!client_) {
      return;
    }

    peer_id_.reset();
    client_ = nullptr;

    logger::get_logger()->debug("core_service_daemon_client is stopped.");
  }

  std::unique_ptr<xpc::client> client_;
  std::optional<xpc::peer_id> peer_id_;
  std::once_flag unregister_callbacks_and_detach_once_;
};
} // namespace krbn
