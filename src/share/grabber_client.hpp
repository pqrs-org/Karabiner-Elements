#pragma once

// `krbn::grabber_client` can be used safely in a multi-threaded environment.

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
class grabber_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>, std::shared_ptr<asio::local::datagram_protocol::endpoint>)> received;
  nod::signal<void(void)> next_heartbeat_deadline_exceeded;

  // Methods

  grabber_client(const grabber_client&) = delete;

  grabber_client(std::optional<std::string> client_socket_directory_name)
      : dispatcher_client(),
        client_socket_directory_name_(client_socket_directory_name) {
  }

  virtual ~grabber_client(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("grabber_client is already started.");
        return;
      }

      prepare_grabber_client_socket_directory();

      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               find_grabber_socket_file_path(),
                                                               grabber_client_socket_file_path(),
                                                               constants::local_datagram_buffer_size);
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));
      client_->set_server_socket_file_path_resolver([this] {
        return find_grabber_socket_file_path();
      });

      client_->connected.connect([this] {
        logger::get_logger()->info("grabber_client is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->error("grabber_client connect_failed: {0}", error_code.message());

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });

        // connect_failed will be triggered if grabber_client_socket_directory does not exist
        // due to the parent directory (system_user_directory) is not ready.
        // For this case, we have to create grabber_client_socket_directory each time.
        prepare_grabber_client_socket_directory();
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("grabber_client is closed.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->error("grabber_client error: {0}", error_code.message());
      });

      client_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
        enqueue_to_dispatcher([this, buffer, sender_endpoint] {
          received(buffer, sender_endpoint);
        });
      });

      client_->next_heartbeat_deadline_exceeded.connect([this](auto&& sender_endpoint) {
        logger::get_logger()->info("grabber_client next heartbeat deadline exceeded");

        enqueue_to_dispatcher([this] {
          next_heartbeat_deadline_exceeded();
        });
      });

      client_->async_start();

      logger::get_logger()->info("grabber_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_connect_console_user_server(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::connect_console_user_server},
          {"user_core_configuration_file_path", constants::get_user_core_configuration_file_path().c_str()},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  void async_system_preferences_updated(std::shared_ptr<pqrs::osx::system_preferences::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::system_preferences_updated},
            {"system_preferences_properties", *properties},
        };

        if (client_) {
          client_->async_send(nlohmann::json::to_msgpack(json));
        }
      }
    });
  }

  void async_frontmost_application_changed(std::shared_ptr<pqrs::osx::frontmost_application_monitor::application> application) const {
    enqueue_to_dispatcher([this, application] {
      if (application) {
        nlohmann::json json{
            {"operation_type", operation_type::frontmost_application_changed},
            {"frontmost_application", *application},
        };

        if (client_) {
          client_->async_send(nlohmann::json::to_msgpack(json));
        }
      }
    });
  }

  void async_input_source_changed(std::shared_ptr<pqrs::osx::input_source::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::input_source_changed},
            {"input_source_properties", *properties},
        };

        if (client_) {
          client_->async_send(nlohmann::json::to_msgpack(json));
        }
      }
    });
  }

  void async_connect_multitouch_extension(void) const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::connect_multitouch_extension},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  void async_set_app_icon(int number) const {
    enqueue_to_dispatcher([this, number] {
      nlohmann::json json{
          {"operation_type", operation_type::set_app_icon},
          {"number", number},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  /**
   * @brief Set variables
   *
   * @param variables nlohmann::json::object which type is {[key: string]: number|boolean|string}.
   * @param processed A callback which is called when the request is processed.
   *                  (When data is sent to grabber or error occurred)
   */
  void async_set_variables(const nlohmann::json& variables,
                           std::function<void(void)> processed = nullptr) const {
    enqueue_to_dispatcher([this, variables, processed] {
      nlohmann::json json{
          {"operation_type", operation_type::set_variables},
          {"variables", variables},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json),
                            processed);
      }
    });
  }

private:
  std::filesystem::path find_grabber_socket_file_path(void) const {
    return filesystem_utility::find_socket_file_path(
        constants::get_grabber_socket_directory_path());
  }

  std::optional<std::filesystem::path> grabber_client_socket_directory_path(void) const {
    if (client_socket_directory_name_ != std::nullopt &&
        client_socket_directory_name_ != "") {
      return constants::get_system_user_directory(geteuid()) / *client_socket_directory_name_;
    }

    return std::nullopt;
  }

  std::optional<std::filesystem::path> grabber_client_socket_file_path(void) const {
    if (auto d = grabber_client_socket_directory_path()) {
      return *d / filesystem_utility::make_socket_file_basename();
    }

    return std::nullopt;
  }

  void prepare_grabber_client_socket_directory(void) {
    if (auto d = grabber_client_socket_directory_path()) {

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

    logger::get_logger()->info("grabber_client is stopped.");
  }

  std::optional<std::string> client_socket_directory_name_;
  std::unique_ptr<pqrs::local_datagram::client> client_;
};
} // namespace krbn
