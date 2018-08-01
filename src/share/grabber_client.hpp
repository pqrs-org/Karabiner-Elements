#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "local_datagram/client_manager.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "shared_instance_provider.hpp"
#include "types.hpp"
#include <unistd.h>
#include <vector>

namespace krbn {
class grabber_client final : public shared_instance_provider<grabber_client> {
public:
  // Signals

  // Note: These signals are fired on local_datagram::client's thread.

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  grabber_client(const grabber_client&) = delete;

  grabber_client(void) : started_(false) {
  }

  void start(void) {
    std::lock_guard<std::mutex> lock(client_manager_mutex_);

    if (started_) {
      return;
    }

    started_ = true;

    client_manager_ = nullptr;

    auto socket_file_path = constants::get_grabber_socket_file_path();
    std::chrono::milliseconds server_check_interval(3000);
    std::chrono::milliseconds reconnect_interval(1000);

    client_manager_ = std::make_unique<local_datagram::client_manager>(socket_file_path,
                                                                       server_check_interval,
                                                                       reconnect_interval);

    client_manager_->connected.connect([this] {
      logger::get_logger().info("grabber_client is connected.");

      connected();
    });

    client_manager_->connect_failed.connect([this](auto&& error_code) {
      connect_failed(error_code);
    });

    client_manager_->closed.connect([this] {
      logger::get_logger().info("grabber_client is closed.");

      closed();
    });

    client_manager_->start();

    logger::get_logger().info("grabber_client is started.");
  }

  void stop(void) {
    std::lock_guard<std::mutex> lock(client_manager_mutex_);

    started_ = false;

    client_manager_ = nullptr;

    logger::get_logger().info("grabber_client is stopped.");
  }

  void grabbable_state_changed(grabbable_state grabbable_state) const {
    operation_type_grabbable_state_changed_struct s;
    s.grabbable_state = grabbable_state;

    async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void connect(void) const {
    operation_type_connect_struct s;
    s.pid = getpid();

    async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void system_preferences_updated(const system_preferences& system_preferences) const {
    operation_type_system_preferences_updated_struct s;
    s.system_preferences = system_preferences;

    async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void frontmost_application_changed(const std::string& bundle_identifier,
                                     const std::string& file_path) const {
    operation_type_frontmost_application_changed_struct s;

    strlcpy(s.bundle_identifier,
            bundle_identifier.c_str(),
            sizeof(s.bundle_identifier));

    strlcpy(s.file_path,
            file_path.c_str(),
            sizeof(s.file_path));

    async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void input_source_changed(const input_source_identifiers& input_source_identifiers) const {
    operation_type_input_source_changed_struct s;

    if (auto& v = input_source_identifiers.get_language()) {
      strlcpy(s.language,
              v->c_str(),
              sizeof(s.language));
    }

    if (auto& v = input_source_identifiers.get_input_source_id()) {
      strlcpy(s.input_source_id,
              v->c_str(),
              sizeof(s.input_source_id));
    }

    if (auto& v = input_source_identifiers.get_input_mode_id()) {
      strlcpy(s.input_mode_id,
              v->c_str(),
              sizeof(s.input_mode_id));
    }

    async_send(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

private:
  void async_send(const uint8_t* _Nonnull p, size_t length) const {
    std::lock_guard<std::mutex> lock(client_manager_mutex_);

    if (client_manager_) {
      if (auto client = client_manager_->get_client()) {
        client->async_send(p, length);
      }
    }
  }

  bool started_;

  std::unique_ptr<local_datagram::client_manager> client_manager_;
  mutable std::mutex client_manager_mutex_;
};
} // namespace krbn
