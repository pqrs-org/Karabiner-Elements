#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "dictionary.hpp"
#include "object.hpp"
#include "options.hpp"
#include <cerrno>
#include <dispatch/dispatch.h>
#include <string>
#include <utility>
#include <xpc/xpc.h>

namespace pqrs::osx::xpc {

// Owns a connection and cancels it before releasing it. Operations, including
// destruction, must be serialized by the caller with the event handler.
class connection final {
public:
  connection() noexcept = default;
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  connection(connection&& other) noexcept
      : value_(std::move(other.value_)),
        resumed_(std::exchange(other.resumed_,
                               false)) {}

  connection& operator=(connection&& other) noexcept {
    if (this != &other) {
      cancel();
      value_ = std::move(other.value_);
      resumed_ = std::exchange(other.resumed_,
                               false);
    }
    return *this;
  }

  ~connection() {
    cancel();
  }

  static connection create_listener(const listener_options& options,
                                    dispatch_queue_t queue) noexcept {
    if (options.listener_parameters.service_name.empty()) {
      // Create an anonymous listener, which cannot be looked up by service name.
      // After listener::listener_started, obtain its endpoint with copy_endpoint()
      // and pass it to client_options::parameters::endpoint to connect.
      // Useful for tests without launchd setup. Cross-process use requires a
      // separate channel to transfer the endpoint (e.g. an existing XPC connection);
      // this byte transport does not provide endpoint transfer itself.
      return connection(adopt_xpc_object(xpc_connection_create(nullptr,
                                                               queue)));
    }

    return create_mach_service(options.listener_parameters.service_name.c_str(),
                               queue,
                               XPC_CONNECTION_MACH_SERVICE_LISTENER);
  }

  static connection create_client(const client_options& options,
                                  dispatch_queue_t queue) noexcept {
    if (options.client_parameters.endpoint.get()) {
      if (xpc_get_type(options.client_parameters.endpoint.get()) != XPC_TYPE_ENDPOINT) {
        return {};
      }

      auto result = connection(adopt_xpc_object(xpc_connection_create_from_endpoint(
          static_cast<xpc_endpoint_t>(options.client_parameters.endpoint.get()))));

      result.set_target_queue(queue);

      return result;
    }

    return create_mach_service(options.client_parameters.service_name.c_str(),
                               queue,
                               options.client_parameters.privileged
                                   ? XPC_CONNECTION_MACH_SERVICE_PRIVILEGED
                                   : 0);
  }

  // Acquires its own reference to a borrowed, newly delivered suspended peer
  // and manages its lifecycle, cancelling it before releasing that reference.
  static connection from_peer(xpc_connection_t peer) noexcept {
    return connection(object(peer));
  }

  [[nodiscard]] xpc_connection_t get() const noexcept {
    return static_cast<xpc_connection_t>(value_.get());
  }

  void set_target_queue(dispatch_queue_t queue) noexcept {
    if (auto connection = get()) {
      xpc_connection_set_target_queue(connection,
                                      queue);
    }
  }

  void set_event_handler(xpc_handler_t handler) noexcept {
    if (auto connection = get()) {
      xpc_connection_set_event_handler(connection,
                                       handler);
    }
  }

  int set_peer_code_signing_requirement(const char* requirement) noexcept {
    if (auto connection = get()) {
      return xpc_connection_set_peer_code_signing_requirement(connection,
                                                              requirement);
    }

    return EINVAL;
  }

  void resume() noexcept {
    if (resumed_) {
      return;
    }

    if (auto connection = get()) {
      xpc_connection_resume(connection);
      resumed_ = true;
    }
  }

  void cancel() noexcept {
    if (auto connection = get()) {
      set_event_handler(^(xpc_object_t){});

      // A suspended connection must be resumed before its final release,
      // including when configuration fails before the first resume.
      resume();

      xpc_connection_cancel(connection);
      value_ = object();
      resumed_ = false;
    }
  }

  [[nodiscard]] object copy_endpoint() const noexcept {
    if (auto connection = get()) {
      return adopt_xpc_object(xpc_endpoint_create(connection));
    }

    return {};
  }

  // true means the native send API was called, not that delivery succeeded.
  bool send_message(const dictionary& message) noexcept {
    if (auto connection = get()) {
      if (auto m = message.get()) {
        xpc_connection_send_message(connection,
                                    m);
        return true;
      }
    }

    return false;
  }

  struct send_message_with_reply_parameters final {
    const dictionary& message;
    dispatch_queue_t queue;
    xpc_handler_t handler;
  };

  // false means no reply handler will be scheduled by this call.
  bool send_message_with_reply(const send_message_with_reply_parameters& parameters) noexcept {
    if (auto connection = get()) {
      if (auto m = parameters.message.get()) {
        xpc_connection_send_message_with_reply(connection,
                                               m,
                                               parameters.queue,
                                               parameters.handler);
        return true;
      }
    }

    return false;
  }

private:
  static connection create_mach_service(const char* name,
                                        dispatch_queue_t queue,
                                        uint64_t flags) noexcept {
    return connection(adopt_xpc_object(xpc_connection_create_mach_service(name,
                                                                          queue,
                                                                          flags)));
  }

  explicit connection(object value) noexcept
      : value_(std::move(value)) {}

  object value_;
  bool resumed_{false};
};
} // namespace pqrs::osx::xpc
