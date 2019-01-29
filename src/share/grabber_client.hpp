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
                                                               socket_file_path);
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

      client_->async_start();

      logger::get_logger()->info("grabber_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_grabbable_state_changed(const grabbable_state& grabbable_state) const {
    enqueue_to_dispatcher([this, grabbable_state] {
      operation_type_grabbable_state_changed_struct s;
      s.grabbable_state = grabbable_state;

      call_async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
    });
  }

  void async_caps_lock_state_changed(bool state) const {
    enqueue_to_dispatcher([this, state] {
      operation_type_caps_lock_state_changed_struct s;
      s.state = state;

      call_async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
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

  void async_system_preferences_updated(const system_preferences& system_preferences) const {
    enqueue_to_dispatcher([this, system_preferences] {
      nlohmann::json json{
          {"operation_type", operation_type::system_preferences_updated},
          {"system_preferences", system_preferences},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
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
