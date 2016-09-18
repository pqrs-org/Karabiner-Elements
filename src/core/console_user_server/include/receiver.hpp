#pragma once

#include "configuration_manager.hpp"
#include "constants.hpp"
#include "grabber_client.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "system_preferences_monitor.hpp"
#include "types.hpp"
#include <vector>

class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) {}

  void start(void) {
    const char* path = constants::get_console_user_socket_file_path();
    unlink(path);
    chmod(path, 0600);

    grabber_client_ = std::make_unique<grabber_client>();
    grabber_client_->connect(krbn::connect_from::console_user_server);

    system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(
        logger::get_logger(),
        std::bind(&receiver::system_preferences_values_updated_callback, this, std::placeholders::_1));

    configuration_manager_ = std::make_unique<configuration_manager>(logger::get_logger(),
                                                                     constants::get_configuration_directory(),
                                                                     *grabber_client_);

    logger::get_logger().info("receiver is started");
  }

  void stop(void) {
    unlink(constants::get_console_user_socket_file_path());

    configuration_manager_.reset(nullptr);
    grabber_client_.reset(nullptr);
    system_preferences_monitor_.reset(nullptr);

    logger::get_logger().info("receiver is stopped");
  }

private:
  void system_preferences_values_updated_callback(const system_preferences::values& values) {
    if (grabber_client_) {
      grabber_client_->system_preferences_values_updated(values);
    }
  }

  std::vector<uint8_t> buffer_;
  std::unique_ptr<grabber_client> grabber_client_;
  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<configuration_manager> configuration_manager_;
};
