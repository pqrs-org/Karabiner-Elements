#pragma once

// `krbn::grabber_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"
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

  // Methods

  grabber_client(const grabber_client&) = delete;

  grabber_client(void) : dispatcher_client() {
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

      auto socket_file_path = constants::get_grabber_socket_file_path();
      client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                               socket_file_path,
                                                               constants::get_local_datagram_buffer_size());
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));

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

      client_->async_start();

      logger::get_logger()->info("grabber_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_key_down_up_valued_event_arrived(device_id device_id,
                                              const key_down_up_valued_event& event,
                                              event_type event_type,
                                              absolute_time_point time_stamp) const {
    enqueue_to_dispatcher([this, device_id, event, event_type, time_stamp] {
      nlohmann::json json{
          {"operation_type", operation_type::key_down_up_valued_event_arrived},
          {"device_id", device_id},
          {"key_down_up_valued_event", event},
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

private:
  void stop(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;

    logger::get_logger()->info("grabber_client is stopped.");
  }

  void call_async_send(const uint8_t* _Nonnull p, size_t length) const {
    if (client_) {
      client_->async_send(p, length);
    }
  }

  std::unique_ptr<pqrs::local_datagram::client> client_;
};
} // namespace krbn
