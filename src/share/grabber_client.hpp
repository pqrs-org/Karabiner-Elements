#pragma once

// `krbn::grabber_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <glob/glob.hpp>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/frontmost_application_monitor.hpp>
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

  // Methods

  grabber_client(const grabber_client&) = delete;

  grabber_client(std::optional<std::string> client_socket_file_path = std::nullopt)
      : dispatcher_client(),
        client_socket_file_path_(client_socket_file_path) {
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

      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               find_grabber_socket_file_path(),
                                                               client_socket_file_path_,
                                                               constants::get_local_datagram_buffer_size());
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
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
        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
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

      client_->async_start();

      logger::get_logger()->info("grabber_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_momentary_switch_event_arrived(device_id device_id,
                                            const momentary_switch_event& event,
                                            event_type event_type,
                                            absolute_time_point time_stamp) const {
    enqueue_to_dispatcher([this, device_id, event, event_type, time_stamp] {
      nlohmann::json json{
          {"operation_type", operation_type::momentary_switch_event_arrived},
          {"device_id", device_id},
          {"momentary_switch_event", event},
          {"event_type", event_type},
          {"time_stamp", time_stamp},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  void async_observed_devices_updated(const std::unordered_set<device_id>& observed_devices) const {
    enqueue_to_dispatcher([this, observed_devices] {
      nlohmann::json json{
          {"operation_type", operation_type::observed_devices_updated},
          {"observed_devices", observed_devices},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

  void async_caps_lock_state_changed(bool state) const {
    enqueue_to_dispatcher([this, state] {
      nlohmann::json json{
          {"operation_type", operation_type::caps_lock_state_changed},
          {"caps_lock_state", state},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
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

  /**
   * @brief Set variables
   *
   * @param variables nlohmann::json::object which type is {[key: string]: number}.
   * @param processed A callback which is called when the request is processed.
   *                  (When data is sent to grabber or error occurred)
   */
  void async_set_variables(const nlohmann::json& variables,
                           const std::function<void(void)>& processed = nullptr) const {
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
    auto pattern = (constants::get_grabber_socket_directory_path() / "*.sock").string();
    auto paths = glob::glob(pattern);
    std::sort(std::begin(paths), std::end(paths));

    if (!paths.empty()) {
      return paths.back();
    }

    return constants::get_grabber_socket_directory_path() / "not_found.sock";
  }

  void stop(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;

    logger::get_logger()->info("grabber_client is stopped.");
  }

  std::optional<std::string> client_socket_file_path_;
  std::unique_ptr<pqrs::local_datagram::client> client_;
};
} // namespace krbn
