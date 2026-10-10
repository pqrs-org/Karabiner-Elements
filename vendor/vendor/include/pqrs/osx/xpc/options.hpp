#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "object.hpp"
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

namespace pqrs::osx::xpc {

struct common_options {
  struct parameters final {
    // Code signing requirement that remote peers must satisfy, in either role.
    // nullopt skips this check and permits unsigned peers for development.
    // An invalid requirement prevents the connection from being used.
    std::optional<std::string> signing_requirement;

    // Maximum payload size in bytes for outgoing and incoming messages/replies.
    // Does not include XPC protocol metadata. Zero permits only empty payloads.
    // nullopt disables this library's limit; system resource limits still apply.
    std::optional<size_t> max_message_size{32 * 1024};

    // Reply deadline for each request, including the initial handshake.
    // Expiration invalidates the peer and fails its pending requests.
    // Must be positive and no greater than one hour.
    // Expiration is checked on the periodic transport tick.
    std::chrono::milliseconds request_timeout{15000};

    // Interval for checking request deadlines and scheduling connection retries.
    // Shorter intervals improve responsiveness but increase timer wakeups.
    // Must be positive and no greater than one hour.
    std::chrono::milliseconds tick_interval{1000};

    [[nodiscard]] bool validate() const noexcept {
      return request_timeout.count() > 0 &&
             request_timeout <= std::chrono::hours{1} &&
             tick_interval.count() > 0 &&
             tick_interval <= std::chrono::hours{1};
    }
  };

  common_options() = default;

  explicit common_options(const parameters& parameters)
      : common_parameters(parameters) {}

  parameters common_parameters;
};

struct listener_options final : public common_options {
  struct parameters final {
    // launchd Mach service name to listen on; empty creates an anonymous listener.
    std::string service_name;
  };

  struct configuration final {
    common_options::parameters common_parameters;
    parameters listener_parameters;
  };

  listener_options() = default;

  explicit listener_options(const common_options::parameters& common_parameters)
      : common_options(common_parameters) {}

  explicit listener_options(const configuration& configuration)
      : common_options(configuration.common_parameters),
        listener_parameters(configuration.listener_parameters) {}

  [[nodiscard]] bool validate() const noexcept {
    return common_parameters.validate();
  }

  parameters listener_parameters;
};

struct client_options final : public common_options {
  struct parameters final {
    // launchd Mach service name to connect to when endpoint is empty.
    std::string service_name;

    // Look up a named service in the privileged bootstrap domain.
    // Ignored when connecting through endpoint.
    bool privileged{false};

    // Delay before reconnecting or retrying an interrupted handshake.
    // Must be positive and no greater than one hour.
    // Retries run on the periodic transport tick.
    std::chrono::milliseconds reconnect_interval{1000};

    // Retained XPC endpoint for connecting directly to an anonymous listener.
    // Takes precedence over service_name and privileged when set.
    // Must contain an XPC endpoint; leave empty for named-service lookup.
    object endpoint{};

    [[nodiscard]] bool validate() const noexcept {
      return reconnect_interval.count() > 0 &&
             reconnect_interval <= std::chrono::hours{1} &&
             (!endpoint ||
              xpc_get_type(endpoint.get()) == XPC_TYPE_ENDPOINT);
    }
  };

  struct configuration final {
    common_options::parameters common_parameters;
    parameters client_parameters;
  };

  client_options() = default;

  explicit client_options(const common_options::parameters& common_parameters)
      : common_options(common_parameters) {}

  explicit client_options(const configuration& configuration)
      : common_options(configuration.common_parameters),
        client_parameters(configuration.client_parameters) {}

  [[nodiscard]] bool validate() const noexcept {
    return common_parameters.validate() &&
           client_parameters.validate();
  }

  parameters client_parameters;
};

} // namespace pqrs::osx::xpc
