#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../types.hpp"
#include <asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pqrs::unix_domain_stream::impl {

class request_manager final {
public:
  request_manager(const request_manager&) = delete;

  request_manager(asio::io_context& io_ctx,
                  dispatcher::extra::dispatcher_client& dispatcher_client)
      : io_ctx_(io_ctx),
        dispatcher_client_(dispatcher_client) {
  }

  // Register a pending request and return the request_id to put into the frame.
  // The callback is always delivered on the dispatcher thread.
  request_id add(std::optional<peer_id> peer_id_value,
                 std::chrono::milliseconds timeout,
                 async_request_callback request_callback,
                 std::function<void()> timeout_callback = nullptr) {
    auto id = ++next_request_id_;
    not_null_shared_ptr_t<asio::steady_timer> timer(std::make_shared<asio::steady_timer>(io_ctx_));
    timer->expires_after(timeout);
    timer->async_wait([this, id, timeout_callback](const auto& error_code) {
      if (!error_code) {
        complete(id,
                 asio::error::timed_out,
                 nullptr);

        if (timeout_callback) {
          timeout_callback();
        }
      }
    });

    pending_requests_.emplace(id,
                              pending_request{
                                  .peer_id_value = peer_id_value,
                                  .callback = request_callback,
                                  .timer = timer,
                              });

    return id;
  }

  // Complete one pending request by request_id. Use this for single-peer owners
  // such as client, where request_id alone identifies the active peer.
  void complete(request_id id,
                const asio::error_code& error_code,
                std::shared_ptr<std::vector<uint8_t>> data) {
    if (auto node = pending_requests_.extract(id);
        !node.empty()) {
      auto request = std::move(node.mapped());

      request.timer->cancel();
      dispatcher_client_.enqueue_to_dispatcher([request, error_code, data] {
        request.callback(error_code,
                         data);
      });
    }
  }

  // Complete one pending request only if it belongs to the given peer. Use this
  // for multi-peer owners such as server, where request_id values are shared
  // across peers on the same manager.
  void complete(peer_id peer_id_value,
                request_id id,
                const asio::error_code& error_code,
                std::shared_ptr<std::vector<uint8_t>> data) {
    if (auto it = pending_requests_.find(id);
        it == std::end(pending_requests_) ||
        it->second.peer_id_value != peer_id_value) {
      return;
    }

    complete(id,
             error_code,
             data);
  }

  // Complete all pending requests associated with one peer, typically when that
  // peer closes or reports an error.
  void complete_peer(peer_id id,
                     const asio::error_code& error_code) {
    for (auto it = std::begin(pending_requests_);
         it != std::end(pending_requests_);) {
      if (it->second.peer_id_value == id) {
        auto node = pending_requests_.extract(it++);
        auto request = std::move(node.mapped());

        request.timer->cancel();
        dispatcher_client_.enqueue_to_dispatcher([request, error_code] {
          request.callback(error_code,
                           nullptr);
        });
      } else {
        ++it;
      }
    }
  }

  // Complete every pending request, typically when the owner is stopping or
  // invalidating all active connections.
  void complete_all(const asio::error_code& error_code) {
    auto pending_requests = std::exchange(pending_requests_,
                                          {});

    for (auto&& [_, request] : pending_requests) {
      request.timer->cancel();
      dispatcher_client_.enqueue_to_dispatcher([request, error_code] {
        request.callback(error_code,
                         nullptr);
      });
    }
  }

private:
  struct pending_request final {
    std::optional<peer_id> peer_id_value;
    async_request_callback callback;
    not_null_shared_ptr_t<asio::steady_timer> timer;
  };

  asio::io_context& io_ctx_;
  dispatcher::extra::dispatcher_client& dispatcher_client_;
  request_id next_request_id_ = 0;
  std::unordered_map<request_id, pending_request> pending_requests_;
};

} // namespace pqrs::unix_domain_stream::impl
