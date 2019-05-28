#pragma once

// `krbn::session_monitor_receiver` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <vector>

namespace krbn {
class session_monitor_receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> current_console_user_id_changed;

  // Methods

  session_monitor_receiver(const receiver&) = delete;

  session_monitor_receiver(void) : dispatcher_client() {
    std::string socket_file_path(constants::get_grabber_session_monitor_receiver_socket_file_path());

    filesystem_utility::mkdir_rootonly_directory();
    unlink(socket_file_path.c_str());

    size_t buffer_size = 32 * 1024;
    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             buffer_size);
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([] {
      logger::get_logger()->info("session_monitor_receiver bound");
    });

    server_->bind_failed.connect([](auto&& error_code) {
      logger::get_logger()->error("session_monitor_receiver bind_failed");
    });

    server_->received.connect([this](auto&& buffer) {
      if (buffer) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          switch (json.at("operation_type").get<operation_type>()) {
            case operation_type::console_user_id_changed: {
              auto new_value = current_console_user_id_;

              auto uid = json.at("user_id").get<uid_t>();

              if (json.at("on_console").get<bool>()) {
                new_value = uid;
              } else {
                if (current_console_user_id_ == uid) {
                  new_value = std::nullopt;
                }
              }

              if (current_console_user_id_ != new_value) {
                current_console_user_id_ = new_value;

                enqueue_to_dispatcher([this, new_value] {
                  current_console_user_id_changed(new_value);
                });
              }

              break;
            }

            default:
              break;
          }
          return;
        } catch (std::exception& e) {
          logger::get_logger()->error("session_monitor_receiver: received data is corrupted");
        }
      }
    });

    logger::get_logger()->info("session_monitor_receiver is initialized");
  }

  virtual ~session_monitor_receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
    });

    logger::get_logger()->info("session_monitor_receiver is terminated");
  }

  void async_start(void) {
    server_->async_start();
  }

private:
  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::optional<uid_t> current_console_user_id_;
};
} // namespace krbn
