#pragma once

#include "filesystem_utility.hpp"
#include <pqrs/local_datagram.hpp>
#include <types/operation_type.hpp>

// `krbn::session_monitor_receiver_client` can be used safely in a multi-threaded environment.

namespace krbn {
class session_monitor_receiver_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const asio::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  session_monitor_receiver_client(const session_monitor_receiver_client&) = delete;

  session_monitor_receiver_client(void) : dispatcher_client() {
  }

  virtual ~session_monitor_receiver_client(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (client_) {
        logger::get_logger()->warn("session_monitor_receiver_client is already started.");
        return;
      }

      prepare_client_socket_directory();

      client_ = std::make_unique<pqrs::local_datagram::client>(
          weak_dispatcher_,
          get_session_monitor_receiver_socket_file_path(),
          get_client_socket_directory() / filesystem_utility::make_socket_file_basename(),
          constants::local_datagram_buffer_size);
      client_->set_server_check_interval(std::chrono::milliseconds(3000));
      client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));
      client_->set_reconnect_interval(std::chrono::milliseconds(1000));
      client_->set_server_socket_file_path_resolver([this] {
        return get_session_monitor_receiver_socket_file_path();
      });

      client_->connected.connect([this](auto&& peer_pid) {
        logger::get_logger()->info("session_monitor_receiver_client is connected.");

        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        logger::get_logger()->info("session_monitor_receiver_client connect_failed: {0}", error_code.message());

        // If the socket directory is deleted for any reason,
        // connect_failed will be triggered, so recreate the directory each time.
        prepare_client_socket_directory();

        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });
      });

      client_->closed.connect([this] {
        logger::get_logger()->info("session_monitor_receiver_client is closed.");

        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      client_->error_occurred.connect([](auto&& error_code) {
        logger::get_logger()->error("session_monitor_receiver_client error: {0}", error_code.message());
      });

      client_->async_start();

      logger::get_logger()->info("session_monitor_receiver_client is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

  void async_console_user_id_changed(uid_t uid,
                                     bool on_console) const {
    enqueue_to_dispatcher([this, uid, on_console] {
      nlohmann::json json{
          {"operation_type", operation_type::console_user_id_changed},
          {"user_id", uid},
          {"on_console", on_console},
      };

      if (client_) {
        client_->async_send(nlohmann::json::to_msgpack(json));
      }
    });
  }

private:
  void prepare_client_socket_directory(void) const {
    // We have to use `getuid` (not `geteuid`) since `karabiner_session_monitor` is run as root by suid.
    // (We have to make a socket directory which includes the real user ID in the file path.)
    filesystem_utility::create_base_directories(getuid());

    auto directory_path = get_client_socket_directory();

    // Remove old socket files.
    std::error_code ec;
    std::filesystem::remove_all(directory_path, ec);
    std::filesystem::create_directory(directory_path, ec);
    chmod(directory_path.c_str(), 0700);
  }

  std::filesystem::path get_client_socket_directory(void) const {
    // We have to use `getuid` (not `geteuid`) since `karabiner_session_monitor` is run as root by suid.
    // (We have to make a socket file which includes the real user ID in the file path.)
    return constants::get_session_monitor_receiver_client_socket_directory_path(getuid());
  }

  std::filesystem::path get_session_monitor_receiver_socket_file_path(void) const {
    return filesystem_utility::find_socket_file_path(
        constants::get_session_monitor_receiver_socket_directory_path());
  }

  void stop(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;

    logger::get_logger()->info("session_monitor_receiver_client is stopped.");
  }

  std::unique_ptr<pqrs::local_datagram::client> client_;
};
} // namespace krbn
