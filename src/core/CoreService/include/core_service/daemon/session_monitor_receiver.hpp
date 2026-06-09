#pragma once

// `krbn::core_service::daemon::session_monitor_receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include <pqrs/osx/session.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <unordered_map>
#include <vector>

namespace krbn {
namespace core_service {
namespace daemon {
class session_monitor_receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> current_console_user_id_changed;

  // Methods

  session_monitor_receiver(const session_monitor_receiver&) = delete;

  session_monitor_receiver(void) : dispatcher_client() {
    prepare_session_monitor_receiver_socket_parent_directory();

    auto options = pqrs::unix_domain_stream::options(
        pqrs::unix_domain_stream::options::initialization_parameters{
            .max_message_size = constants::unix_domain_stream_max_message_size,
            .reconnect_interval = std::chrono::milliseconds(1000),
            .server_check_interval = std::chrono::milliseconds(3000),
        });

    server_ = std::make_unique<pqrs::unix_domain_stream::server>(
        weak_dispatcher_,
        constants::get_session_monitor_receiver_socket_file_path(),
        options,
        [](const auto& peer_credentials) {
          auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          if (!result) {
            logger::get_logger()->warn("session_monitor_receiver: peer is not code-signed with same Team ID");
          }
          return result;
        });

    server_->bound.connect([] {
      logger::get_logger()->info("session_monitor_receiver: bound");
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("session_monitor_receiver: bind_failed");

      // If the socket parent directory is deleted for any reason,
      // bind_failed will be triggered, so recreate the directory each time.
      prepare_session_monitor_receiver_socket_parent_directory();
    });

    server_->closed.connect([] {
      logger::get_logger()->info("session_monitor_receiver: closed");
    });

    server_->peer_connected.connect([](auto peer_id, auto&&) {
      logger::get_logger()->info("session_monitor_receiver: peer_connected ({0})", peer_id);
    });

    server_->peer_closed.connect([this](auto peer_id) {
      logger::get_logger()->info("session_monitor_receiver: peer_closed ({0})", peer_id);

      if (auto it = session_monitor_peer_states_.find(peer_id);
          it != std::end(session_monitor_peer_states_)) {
        auto state = it->second;
        session_monitor_peer_states_.erase(it);

        if (state.on_console &&
            current_console_user_id_ == state.uid &&
            !has_on_console_peer(state.uid)) {
          update_current_console_user_id(std::nullopt);
        }
      }
    });

    server_->peer_error_occurred.connect([](auto peer_id, auto&& error_code) {
      logger::get_logger()->error("session_monitor_receiver: peer_error_occurred ({0}): {1}", peer_id, error_code.message());
    });

    server_->request_received.connect([this](auto peer_id, auto request_id, auto&& buffer) {
      if (buffer->empty()) {
        server_->async_close_peer(peer_id);
        return;
      }

      try {
        nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
        switch (json.at("operation_type").get<operation_type>()) {
          case operation_type::console_user_id_changed: {
            auto uid = json.at("user_id").get<uid_t>();
            auto on_console = json.at("on_console").get<bool>();

            session_monitor_peer_states_[peer_id] = session_monitor_peer_state{
                .uid = uid,
                .on_console = on_console,
            };

            if (on_console) {
              update_current_console_user_id(uid);
            } else {
              if (current_console_user_id_ == uid) {
                if (!has_on_console_peer(uid)) {
                  update_current_console_user_id(std::nullopt);
                }
              }
            }

            server_->async_respond(peer_id, request_id, {});

            break;
          }

          default:
            server_->async_close_peer(peer_id);
            break;
        }
        return;
      } catch (std::exception& e) {
        logger::get_logger()->error("session_monitor_receiver: received data is corrupted");
      }

      server_->async_close_peer(peer_id);
    });

    logger::get_logger()->info("session_monitor_receiver is initialized");
  }

  virtual ~session_monitor_receiver(void) {
    detach_from_dispatcher([this] {
      session_monitor_peer_states_.clear();
      server_ = nullptr;
    });

    logger::get_logger()->info("session_monitor_receiver is terminated");
  }

  void async_start(void) {
    server_->async_start();
  }

private:
  struct session_monitor_peer_state final {
    uid_t uid;
    bool on_console;
  };

  void prepare_session_monitor_receiver_socket_parent_directory(void) const {
    filesystem_utility::create_base_directories(std::nullopt);
  }

  bool has_on_console_peer(uid_t uid) const {
    for (const auto& [_, state] : session_monitor_peer_states_) {
      if (state.uid == uid &&
          state.on_console) {
        return true;
      }
    }

    return false;
  }

  void update_current_console_user_id(std::optional<uid_t> value) {
    if (current_console_user_id_ != value) {
      current_console_user_id_ = value;

      filesystem_utility::create_base_directories(value);

      enqueue_to_dispatcher([this, value] {
        current_console_user_id_changed(value);
      });
    }
  }

  std::unique_ptr<pqrs::unix_domain_stream::server> server_;
  std::optional<uid_t> current_console_user_id_;
  std::unordered_map<pqrs::unix_domain_stream::peer_id, session_monitor_peer_state> session_monitor_peer_states_;
};
} // namespace daemon
} // namespace core_service
} // namespace krbn
