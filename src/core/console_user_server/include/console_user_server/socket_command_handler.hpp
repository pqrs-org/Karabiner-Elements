#pragma once

#include "logger.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/dispatcher.hpp>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

namespace krbn {
namespace console_user_server {

class socket_command_handler final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  socket_command_handler(const socket_command_handler&) = delete;

  socket_command_handler(void)
      : dispatcher_client() {
  }

  ~socket_command_handler(void) {
    detach_from_dispatcher([this] {
      close_all_connections();
    });
  }

  void run(const std::string& socket_command_json) {
    enqueue_to_dispatcher([this, socket_command_json] {
      try {
        auto json = nlohmann::json::parse(socket_command_json);

        if (!json.contains("endpoint")) {
          logger::get_logger()->error("socket_command: missing 'endpoint' field");
          return;
        }
        if (!json.contains("command")) {
          logger::get_logger()->error("socket_command: missing 'command' field");
          return;
        }

        auto endpoint = json.at("endpoint").get<std::string>();
        auto command = json.at("command").get<std::string>();

        std::string payload = command + "\n";

        // Try sending on existing persistent connection first
        int fd = get_or_connect(endpoint);
        if (fd >= 0 && send_all(fd, payload)) {
            logger::get_logger()->info("socket_command: sent '{}' to {}", command, endpoint);
            return;
        }

        if (fd >= 0) {
          // Connection broken (EPIPE, ECONNRESET, etc.) — close and reconnect.
          close_connection(endpoint);
        }

        // Reconnect and retry once
        fd = connect_to(endpoint);
        if (fd < 0) {
          return;
        }

        if (!send_all(fd, payload)) {
          logger::get_logger()->error("socket_command: write() failed after reconnect: {}", strerror(errno));
          close_connection(endpoint);
        } else {
          logger::get_logger()->info("socket_command: sent '{}' to {} (reconnected)", command, endpoint);
        }

      } catch (const std::exception& e) {
        logger::get_logger()->error("socket_command error: {}", e.what());
      }
    });
  }

private:
  static bool send_all(int fd, const std::string& payload) {
    const char* data = payload.data();
    size_t size = payload.size();
    size_t offset = 0;

    while (offset < size) {
      auto n = ::send(fd, data + offset, size - offset, 0);
      if (n > 0) {
        offset += static_cast<size_t>(n);
        continue;
      }

      if (n == 0) {
        errno = ECONNRESET;
        return false;
      }

      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    return true;
  }

  int get_or_connect(const std::string& endpoint) {
    auto it = endpoint_fds_.find(endpoint);
    if (it != endpoint_fds_.end()) {
      return it->second;
    }
    return connect_to(endpoint);
  }

  int connect_to(const std::string& endpoint) {
    if (endpoint.size() >= sizeof(((struct sockaddr_un*)nullptr)->sun_path)) {
      logger::get_logger()->error("socket_command: endpoint path too long: {}", endpoint);
      return -1;
    }

    // Defensive: avoid fd leaks if connect_to is called redundantly.
    close_connection(endpoint);

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
      logger::get_logger()->error("socket_command: socket() failed: {}", strerror(errno));
      return -1;
    }

    // Avoid leaking the fd into spawned processes (shell_command handler forks).
    if (auto flags = fcntl(sock, F_GETFD, 0); flags >= 0) {
      fcntl(sock, F_SETFD, flags | FD_CLOEXEC);
    }

    // Prevent SIGPIPE on broken connection (macOS does not support MSG_NOSIGNAL).
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)) != 0) {
      logger::get_logger()->error("socket_command: setsockopt(SO_NOSIGPIPE) failed: {}", strerror(errno));
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, endpoint.c_str(), sizeof(addr.sun_path) - 1);

    auto addr_len = static_cast<socklen_t>(offsetof(struct sockaddr_un, sun_path) + endpoint.size() + 1);
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), addr_len) < 0) {
      logger::get_logger()->error("socket_command: connect() to {} failed: {}", endpoint, strerror(errno));
      close(sock);
      return -1;
    }

    endpoint_fds_[endpoint] = sock;
    return sock;
  }

  void close_connection(const std::string& endpoint) {
    auto it = endpoint_fds_.find(endpoint);
    if (it != endpoint_fds_.end()) {
      close(it->second);
      endpoint_fds_.erase(it);
    }
  }

  void close_all_connections(void) {
    for (auto& [endpoint, fd] : endpoint_fds_) {
      close(fd);
    }
    endpoint_fds_.clear();
  }

  std::unordered_map<std::string, int> endpoint_fds_;
};

} // namespace console_user_server
} // namespace krbn
