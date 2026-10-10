#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "connection.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <pqrs/gsl.hpp>
#include <unordered_map>
#include <utility>

namespace pqrs::osx::xpc {

// Transport protocol state, accessed only on the owning transport queue.
class peer final {
public:
  struct session_identity final {};

  struct pending_request {
    pqrs::osx::xpc::completion completion;
    std::chrono::steady_clock::time_point deadline;
    bool handshake;
  };

  explicit peer(connection connection)
      : connection_(std::move(connection)) {}

  [[nodiscard]] connection& get_connection() noexcept {
    return connection_;
  }

  [[nodiscard]] const connection& get_connection() const noexcept {
    return connection_;
  }

  [[nodiscard]] bool ready_for_communication() const noexcept {
    return ready_for_communication_;
  }

  bool mark_ready_for_communication() noexcept {
    if (ready_for_communication_) {
      return false;
    }

    ready_for_communication_ = true;
    connection_interrupted_ = false;
    return true;
  }

  bool mark_connection_interrupted(std::chrono::steady_clock::time_point handshake_retry_at) {
    if (connection_interrupted_) {
      return false;
    }

    session_ = std::make_shared<session_identity>();
    connection_interrupted_ = true;
    ready_for_communication_ = false;
    handshake_retry_at_ = handshake_retry_at;
    return true;
  }

  [[nodiscard]] std::weak_ptr<session_identity> get_session() const noexcept {
    return session_.get();
  }

  [[nodiscard]] bool matches_session(const std::weak_ptr<session_identity>& session) const noexcept {
    return session.lock() == session_.get();
  }

  void schedule_handshake_retry(std::chrono::steady_clock::time_point handshake_retry_at) noexcept {
    handshake_retry_at_ = handshake_retry_at;
  }

  [[nodiscard]] bool can_retry_handshake(std::chrono::steady_clock::time_point now) const noexcept {
    return !ready_for_communication_ &&
           pending_requests_.empty() &&
           now >= handshake_retry_at_;
  }

  [[nodiscard]] bool has_expired_pending_request(std::chrono::steady_clock::time_point now) const noexcept {
    return std::ranges::any_of(pending_requests_,
                               [&](const auto& request) {
                                 return now >= request.second.deadline;
                               });
  }

  [[nodiscard]] size_t get_pending_request_count() const noexcept {
    return pending_requests_.size();
  }

  void add_pending_request(uint64_t id, pending_request request) {
    pending_requests_.emplace(id,
                              std::move(request));
  }

  std::optional<pending_request> take_pending_request(uint64_t id) {
    if (auto it = pending_requests_.find(id); it != pending_requests_.end()) {
      auto result = std::move(it->second);
      pending_requests_.erase(it);
      return result;
    }

    return std::nullopt;
  }

  std::unordered_map<uint64_t, pending_request> take_pending_requests() {
    return std::exchange(pending_requests_,
                         {});
  }

private:
  connection connection_;

  // The transport has established that this peer can exchange application messages.
  // Set by the handshake, or by an authenticated server message received before
  // the handshake reply. Cleared when the connection is interrupted.
  bool ready_for_communication_{false};

  // An interruption has been handled and communication has not become ready again.
  // XPC reports a connection interruption when the remote service exits, e.g.
  // after a crash or restart. A named-service connection remains reusable:
  // a later send can launch the service again. The transport clears readiness,
  // replaces the session to invalidate old reply tokens, fails pending requests,
  // and retries the handshake on the client side.
  // Only connection-event interruptions set this flag. A reply-handler
  // interruption means that request will never receive a reply; it does not
  // necessarily mean the whole connection was interrupted.
  // Suppresses repeated interruption handling until mark_ready_for_communication() clears it.
  bool connection_interrupted_{false};

  // Unique identity for this peer's current session. Reply tokens hold a weak
  // reference to it; pointer identity rejects tokens from other peers/transports.
  // Replaced on interruption to invalidate old tokens even when XPC reuses the
  // connection. Removing the peer also invalidates its tokens.
  pqrs::not_null_shared_ptr_t<session_identity> session_{std::make_shared<session_identity>()};

  std::chrono::steady_clock::time_point handshake_retry_at_{};

  // Tracks pending callbacks and deadlines locally.
  // XPC handles request/reply matching and reply delivery.
  std::unordered_map<uint64_t, pending_request> pending_requests_;
};

} // namespace pqrs::osx::xpc
