#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "peer.hpp"
#include "types.hpp"
#include <chrono>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pqrs::osx::xpc {

// Owns peer connections and allocates IDs that are not reused after removal.
// Reuse after uint64_t wraparound is intentionally permitted.
// Peer count is intentionally unlimited. In normal production use, a signing
// requirement restricts admission to trusted processes and limits untrusted
// processes from consuming peer resources.
// All operations and access to returned peers must run on the transport queue.
class peers final {
public:
  std::pair<peer_id, peer&> add(connection connection) {
    // Peer ID wraparound is intentionally permitted.
    auto id = ++next_peer_id_;
    auto [it, inserted] = peers_.try_emplace(id,
                                             std::move(connection));
    return {id, it->second};
  }

  [[nodiscard]] peer* find(peer_id id) noexcept {
    auto it = peers_.find(id);
    return it != peers_.end()
               ? &it->second
               : nullptr;
  }

  [[nodiscard]] const peer* find(peer_id id) const noexcept {
    auto it = peers_.find(id);
    return it != peers_.end()
               ? &it->second
               : nullptr;
  }

  void erase(peer_id id) {
    peers_.erase(id);
  }

  void clear() noexcept {
    peers_.clear();
  }

  [[nodiscard]] bool empty() const noexcept {
    return peers_.empty();
  }

  struct tick_actions final {
    std::vector<peer_id> expired_peer_ids;
    std::vector<peer_id> handshake_retry_target_peer_ids;
  };

  // Return IDs so the caller can act on them without iterating the peer map.
  [[nodiscard]] tick_actions collect_tick_actions(std::chrono::steady_clock::time_point now) const {
    tick_actions result;
    for (const auto& [id, peer] : peers_) {
      if (peer.has_expired_pending_request(now)) {
        result.expired_peer_ids.push_back(id);
      }

      if (peer.can_retry_handshake(now)) {
        result.handshake_retry_target_peer_ids.push_back(id);
      }
    }
    return result;
  }

private:
  std::unordered_map<peer_id, peer> peers_;
  peer_id next_peer_id_{0};
};

} // namespace pqrs::osx::xpc
