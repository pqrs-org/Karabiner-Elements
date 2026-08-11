#pragma once

// `krbn::core_service::daemon::console_user_id_changed_receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include <atomic>
#include <pqrs/unix_domain_stream.hpp>
#include <unordered_map>
#include <vector>

namespace krbn::core_service::daemon {
class console_user_id_changed_receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> current_console_user_id_changed;

  // Methods

  console_user_id_changed_receiver(const console_user_id_changed_receiver&) = delete;

  console_user_id_changed_receiver() : dispatcher_client() {
    prepare_console_user_id_changed_receiver_socket_parent_directory();

    server_ = std::make_unique<pqrs::unix_domain_stream::server>(
        weak_dispatcher_,
        constants::get_console_user_id_changed_receiver_socket_file_path(),
        constants::get_unix_domain_stream_server_options(),
        [](const auto& peer_credentials) {
          auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          if (!result) {
            // During an update, retrieving the Team ID may fail, causing an error once.
            // Since this can occur during normal use, treat it as debug rather than warn.
            logger::get_logger()->debug("console_user_id_changed_receiver: peer is not code-signed with same Team ID (pid: {0})",
                                        peer_credentials.pid.value_or(-1));
          }
          return result;
        });

    server_->bound.connect([] {
      logger::get_logger()->debug("console_user_id_changed_receiver: bound");

      if (!filesystem_utility::permissions(constants::get_console_user_id_changed_receiver_socket_file_path(),
                                           filesystem_utility::permissions_0666)) {
        return;
      }
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("console_user_id_changed_receiver: bind_failed");

      // If the socket parent directory is deleted for any reason,
      // bind_failed will be triggered, so recreate the directory each time.
      prepare_console_user_id_changed_receiver_socket_parent_directory();
    });

    server_->closed.connect([] {
      logger::get_logger()->debug("console_user_id_changed_receiver: closed");
    });

    server_->peer_connected.connect([this](auto peer_id, const auto& peer_credentials) {
      peer_credentials_[peer_id] = peer_credentials;
    });

    server_->peer_closed.connect([this](auto peer_id) {
      peer_credentials_.erase(peer_id);

      if (auto it = peer_states_.find(peer_id);
          it != std::end(peer_states_)) {
        auto state = it->second;
        peer_states_.erase(it);

        if (state.on_console &&
            current_console_user_id_ == state.uid &&
            !has_on_console_peer(state.uid)) {
          update_current_console_user_id(std::nullopt);
        }
      }
    });

    server_->peer_error_occurred.connect([](auto peer_id, auto&& error_code) {
      logger::get_logger()->debug("console_user_id_changed_receiver: peer_error_occurred ({0}): {1}", peer_id, error_code.message());
    });

    server_->received.connect([](auto, auto&&) {
      // Do nothing
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
            auto uid = get_peer_uid(peer_id);
            if (!uid) {
              server_->async_close_peer(peer_id);
              break;
            }

            auto on_console = json.at("on_console").get<bool>();

            peer_states_[peer_id] = peer_state{
                .uid = *uid,
                .on_console = on_console,
            };

            if (on_console) {
              update_current_console_user_id(*uid);
            } else {
              if (current_console_user_id_ == uid) {
                if (!has_on_console_peer(*uid)) {
                  update_current_console_user_id(std::nullopt);
                }
              }
            }

            // This response means that the request has been accepted.
            // The applied state is sent separately by core_service_daemon_server_bound.
            server_->async_respond(peer_id, request_id, {});

            // Send the current bound state to the requester even if the console user ID
            // did not change and the receiver was not recreated.
            send_core_service_daemon_server_bound_if_available(peer_id);

            break;
          }

          default:
            server_->async_close_peer(peer_id);
            break;
        }
        return;
      } catch (std::exception& e) {
        logger::get_logger()->error("console_user_id_changed_receiver: received data is corrupted");
      }

      server_->async_close_peer(peer_id);
    });

    logger::get_logger()->debug("console_user_id_changed_receiver is initialized");
  }

  ~console_user_id_changed_receiver() override {
    detach_from_dispatcher([this] {
      peer_credentials_.clear();
      peer_states_.clear();
      server_ = nullptr;
    });

    logger::get_logger()->debug("console_user_id_changed_receiver is terminated");
  }

  void async_start() {
    server_->async_start();
  }

  void async_core_service_daemon_server_bound(std::optional<uid_t> uid) {
    auto generation = ++core_service_daemon_server_state_generation_;

    enqueue_to_dispatcher([this, uid, generation] {
      if (generation != core_service_daemon_server_state_generation_ ||
          uid != current_console_user_id_) {
        return;
      }

      core_service_daemon_server_is_bound_ = true;
      core_service_daemon_server_bound_uid_ = uid;

      broadcast_core_service_daemon_server_bound();
    });
  }

private:
  struct peer_state final {
    uid_t uid;
    bool on_console;
  };

  void prepare_console_user_id_changed_receiver_socket_parent_directory() const {
    filesystem_utility::prepare_system_directories(std::nullopt);
  }

  [[nodiscard]] std::optional<uid_t> get_peer_uid(pqrs::unix_domain_stream::peer_id peer_id) const {
    if (auto it = peer_credentials_.find(peer_id);
        it != std::end(peer_credentials_)) {
      return it->second.uid;
    }

    return std::nullopt;
  }

  [[nodiscard]] bool has_on_console_peer(uid_t uid) const {
    for (const auto& [_, state] : peer_states_) {
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
      ++core_service_daemon_server_state_generation_;
      core_service_daemon_server_is_bound_ = false;

      filesystem_utility::prepare_system_directories(value);

      enqueue_to_dispatcher([this, value] {
        current_console_user_id_changed(value);
      });
    }
  }

  void send_core_service_daemon_server_bound_if_available(pqrs::unix_domain_stream::peer_id peer_id) const {
    if (!server_ ||
        !core_service_daemon_server_is_bound_) {
      return;
    }

    nlohmann::json uid = nullptr;
    if (core_service_daemon_server_bound_uid_) {
      uid = *core_service_daemon_server_bound_uid_;
    }

    nlohmann::json json{
        {"operation_type", operation_type::core_service_daemon_server_bound},
        {"uid", uid},
    };

    server_->async_send(peer_id,
                        nlohmann::json::to_msgpack(json));
  }

  void broadcast_core_service_daemon_server_bound() const {
    for (const auto& [peer_id, _] : peer_credentials_) {
      send_core_service_daemon_server_bound_if_available(peer_id);
    }
  }

  std::unique_ptr<pqrs::unix_domain_stream::server> server_;
  std::optional<uid_t> current_console_user_id_;
  std::atomic<std::size_t> core_service_daemon_server_state_generation_ = 0;

  // A null bound UID is a valid state when there is no console user, so it cannot
  // also represent that the server has not been bound yet. Track bound separately.
  bool core_service_daemon_server_is_bound_ = false;
  std::optional<uid_t> core_service_daemon_server_bound_uid_;
  std::unordered_map<pqrs::unix_domain_stream::peer_id, pqrs::unix_domain_stream::peer_credentials> peer_credentials_;
  std::unordered_map<pqrs::unix_domain_stream::peer_id, peer_state> peer_states_;
};
} // namespace krbn::core_service::daemon
