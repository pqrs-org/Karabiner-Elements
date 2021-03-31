#pragma once

// `krbn::grabber::session_monitor_receiver` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <vector>

namespace krbn {
namespace grabber {
class session_monitor_receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> current_console_user_id_changed;

  // Methods

  session_monitor_receiver(const session_monitor_receiver&) = delete;

  session_monitor_receiver(void) : dispatcher_client() {
    // Remove old socket files.
    auto socket_directory_path = constants::get_grabber_session_monitor_receiver_socket_directory_path();
    {
      std::error_code ec;
      std::filesystem::remove_all(socket_directory_path, ec);
      std::filesystem::create_directory(socket_directory_path, ec);
      chmod(socket_directory_path.c_str(), 0700);
    }

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_directory_path / filesystem_utility::make_socket_file_basename(),
                                                             constants::get_local_datagram_buffer_size());
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([] {
      logger::get_logger()->info("session_monitor_receiver: bound");
    });

    server_->bind_failed.connect([](auto&& error_code) {
      logger::get_logger()->error("session_monitor_receiver: bind_failed");
    });

    server_->closed.connect([] {
      logger::get_logger()->info("session_monitor_receiver: closed");
    });

    server_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
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

              filesystem_utility::mkdir_system_user_directory(uid);

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

              // manage session_monitor_client_

              register_session_monitor_client(uid);

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
      session_monitor_clients_.clear();
      server_ = nullptr;
    });

    logger::get_logger()->info("session_monitor_receiver is terminated");
  }

  void async_start(void) {
    server_->async_start();
  }

private:
  void register_session_monitor_client(uid_t uid) {
    if (session_monitor_clients_.find(uid) == std::end(session_monitor_clients_)) {
      auto socket_file_path = constants::get_session_monitor_receiver_socket_file_path(uid);
      auto client = std::make_shared<pqrs::local_datagram::client>(weak_dispatcher_,
                                                                   socket_file_path,
                                                                   std::nullopt,
                                                                   constants::get_local_datagram_buffer_size());
      session_monitor_clients_[uid] = client;

      client->set_server_check_interval(std::chrono::milliseconds(3000));

      client->closed.connect([this, uid] {
        logger::get_logger()->info("session_monitor_client is closed (uid:{0})", uid);

        if (current_console_user_id_ == uid) {
          current_console_user_id_ = std::nullopt;

          enqueue_to_dispatcher([this] {
            current_console_user_id_changed(current_console_user_id_);
          });
        }

        enqueue_to_dispatcher([this, uid] {
          session_monitor_clients_.erase(uid);
        });
      });

      client->async_start();
    }
  }

  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::optional<uid_t> current_console_user_id_;
  std::unordered_map<uid_t, std::shared_ptr<pqrs::local_datagram::client>> session_monitor_clients_;
};
} // namespace grabber
} // namespace krbn
