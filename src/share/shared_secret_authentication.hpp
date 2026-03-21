#pragma once

#include "codesign_manager.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <functional>
#include <pqrs/local_datagram.hpp>

namespace krbn {
namespace shared_secret_authentication {

class client final {
public:
  void connected(pqrs::local_datagram::client& client) {
    reset();

    nlohmann::json json{
        {"operation_type", operation_type::handshake},
    };
    client.async_send(nlohmann::json::to_msgpack(json));
  }

  bool handle_shared_secret_payload(const std::vector<uint8_t>& buffer,
                                    pqrs::local_datagram::client& client,
                                    std::function<void(void)> authenticated) {
    auto json = nlohmann::json::from_msgpack(buffer);
    switch (json.at("operation_type").get<operation_type>()) {
      case operation_type::shared_secret:
        shared_secret_ = json.at("shared_secret").get<std::vector<uint8_t>>();
        flush_pending_messages(client);
        authenticated();
        return true;

      default:
        return false;
    }
  }

  void async_send_message(pqrs::local_datagram::client& client,
                          nlohmann::json&& json,
                          std::function<void(void)> processed = nullptr) {
    if (shared_secret_.empty()) {
      pending_messages_.push_back(pending_message{std::move(json), processed});
      return;
    }

    json["shared_secret"] = shared_secret_;

    client.async_send(nlohmann::json::to_msgpack(json),
                      processed);
  }

  void reset(void) {
    shared_secret_.clear();
    pending_messages_.clear();
  }

private:
  struct pending_message final {
    nlohmann::json json;
    std::function<void(void)> processed;
  };

  void flush_pending_messages(pqrs::local_datagram::client& client) {
    if (shared_secret_.empty()) {
      return;
    }

    auto pending_messages = std::move(pending_messages_);
    pending_messages_.clear();

    for (auto& message : pending_messages) {
      async_send_message(client,
                         std::move(message.json),
                         message.processed);
    }
  }

  std::vector<uint8_t> shared_secret_;
  std::vector<pending_message> pending_messages_;
};

class receiver final {
public:
  receiver(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
           size_t buffer_size)
      : verified_peer_manager_(std::make_unique<pqrs::local_datagram::extra::peer_manager>(
            weak_dispatcher,
            buffer_size,
            [](auto&& peer_pid,
               auto&& peer_socket_file_path) {
              if (get_shared_codesign_manager()->same_team_id(peer_pid)) {
                logger::get_logger()->info("verified peer connected");
                return true;
              } else {
                logger::get_logger()->warn("peer is not code-signed with same Team ID");
                return false;
              }
            })) {
  }

  bool handle_handshake(const std::filesystem::path& peer_socket_file_path) const {
    if (!verified_peer_manager_) {
      return false;
    }

    std::vector<uint8_t> shared_secret(32);
    arc4random_buf(shared_secret.data(), shared_secret.size());

    verified_peer_manager_->insert_shared_secret(peer_socket_file_path,
                                                 shared_secret);

    nlohmann::json response{
        {"operation_type", operation_type::shared_secret},
        {"shared_secret", shared_secret},
    };
    verified_peer_manager_->async_send(peer_socket_file_path,
                                       nlohmann::json::to_msgpack(response));
    return true;
  }

  bool verify_shared_secret(const std::filesystem::path& peer_socket_file_path,
                            const nlohmann::json& json,
                            operation_type operation_type) const {
    if (!verified_peer_manager_) {
      return false;
    }

    try {
      if (verified_peer_manager_->verify_shared_secret(peer_socket_file_path,
                                                       json.at("shared_secret").get<std::vector<uint8_t>>())) {
        return true;
      }
    } catch (std::exception& e) {
    }

    logger::get_logger()->error("operation_type::{0} with invalid shared secret",
                                nlohmann::json(operation_type).get<std::string>());
    return false;
  }

  void async_send(const std::filesystem::path& peer_socket_file_path,
                  const nlohmann::json& json) const {
    if (verified_peer_manager_) {
      verified_peer_manager_->async_send(peer_socket_file_path,
                                         nlohmann::json::to_msgpack(json));
    }
  }

private:
  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> verified_peer_manager_;
};

} // namespace shared_secret_authentication
} // namespace krbn
