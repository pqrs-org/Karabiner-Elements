#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <cstddef>

namespace pqrs::unix_domain_stream {

struct options final {
  struct initialization_parameters final {
    // Soft limit for one application message payload.
    // This prevents excessive memory use when sending or receiving unexpectedly large frames.
    size_t max_message_size = 32 * 1024;

    // Maximum number of unsent frames kept in the per-peer write queue.
    // This limits memory growth when the peer is slow or the caller sends faster
    // than the socket can write.
    size_t max_send_queue_size = 1024;

    // Interval used to retry client connect or server bind after a failure.
    std::chrono::milliseconds reconnect_interval = std::chrono::milliseconds(1000);

    // Interval used by the server to verify that its socket path is still
    // connectable and that the accepted connection can exchange an internal
    // health-check frame.
    std::chrono::milliseconds server_check_interval = std::chrono::milliseconds(3000);

    // Maximum time allowed for one server health check.
    std::chrono::milliseconds server_check_timeout = std::chrono::milliseconds(1000);

    // Interval used to send heartbeat frames to the peer.
    std::chrono::milliseconds heartbeat_interval = std::chrono::milliseconds(3000);

    // Maximum idle time allowed without receiving any frame from the peer.
    // Heartbeat, health-check and user-data frames all refresh this deadline.
    std::chrono::milliseconds heartbeat_timeout = std::chrono::milliseconds(10000);

    // Maximum time allowed for one async read operation.
    std::chrono::milliseconds read_timeout = std::chrono::milliseconds(5000);

    // Maximum time allowed for one async write operation.
    std::chrono::milliseconds write_timeout = std::chrono::milliseconds(5000);
  };

  options() : options(initialization_parameters{}) {
  }

  explicit options(const initialization_parameters& parameters)
      : max_message_size(parameters.max_message_size),
        max_send_queue_size(parameters.max_send_queue_size),
        reconnect_interval(parameters.reconnect_interval),
        server_check_interval(parameters.server_check_interval),
        server_check_timeout(parameters.server_check_timeout),
        heartbeat_interval(parameters.heartbeat_interval),
        heartbeat_timeout(parameters.heartbeat_timeout),
        read_timeout(parameters.read_timeout),
        write_timeout(parameters.write_timeout) {
  }

  size_t max_message_size;
  size_t max_send_queue_size;
  std::chrono::milliseconds reconnect_interval;
  std::chrono::milliseconds server_check_interval;
  std::chrono::milliseconds server_check_timeout;
  std::chrono::milliseconds heartbeat_interval;
  std::chrono::milliseconds heartbeat_timeout;
  std::chrono::milliseconds read_timeout;
  std::chrono::milliseconds write_timeout;
};

} // namespace pqrs::unix_domain_stream
