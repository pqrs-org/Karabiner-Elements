#pragma once

// Thread-safe, synchronous Unix domain socket sender with persistent connections.
// Designed for low-latency fire-and-forget messaging to local daemons (e.g. seqd).
// Does NOT require a dispatcher thread — can be called from any context.

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

namespace krbn {

class socket_sender final {
public:
  socket_sender() = default;

  ~socket_sender() {
    std::lock_guard<std::mutex> lock(mutex_);
    close_all();
  }

  socket_sender(const socket_sender&) = delete;
  socket_sender& operator=(const socket_sender&) = delete;

  // Send command+newline to endpoint. Returns true on success.
  // Maintains persistent connections with auto-reconnect on broken pipe.
  bool send(const std::string& endpoint, const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string payload = command + "\n";

    int fd = get_or_connect(endpoint);
    if (fd >= 0 && send_all(fd, payload)) {
      return true;
    }

    if (fd >= 0) {
      close_fd(endpoint);
    }

    // Reconnect and retry once
    fd = connect_to(endpoint);
    if (fd < 0) {
      return false;
    }

    if (!send_all(fd, payload)) {
      close_fd(endpoint);
      return false;
    }

    return true;
  }

  // Fire-and-forget DGRAM send. Appends ".dgram" to endpoint path.
  // No connection needed — each message is independent.
  // Falls back to STREAM if DGRAM socket doesn't exist.
  bool send_dgram(const std::string& endpoint, const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string dgram_endpoint = endpoint + ".dgram";
    if (dgram_endpoint.size() >= sizeof(((struct sockaddr_un*)nullptr)->sun_path)) {
      return false;
    }

    std::string payload = command + "\n";

    // Reuse cached DGRAM fd if available.
    int fd = -1;
    auto it = dgram_fds_.find(endpoint);
    if (it != dgram_fds_.end()) {
      fd = it->second;
    } else {
      fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
      if (fd < 0) {
        return false;
      }
      if (auto flags = ::fcntl(fd, F_GETFD, 0); flags >= 0) {
        ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
      }
      int optval = 1;
      ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
      dgram_fds_[endpoint] = fd;
    }

    struct sockaddr_un addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, dgram_endpoint.c_str(), sizeof(addr.sun_path) - 1);
    auto addr_len = static_cast<socklen_t>(
        offsetof(struct sockaddr_un, sun_path) + dgram_endpoint.size() + 1);

    auto n = ::sendto(fd, payload.data(), payload.size(), 0,
                      reinterpret_cast<struct sockaddr*>(&addr), addr_len);
    return n >= 0 && static_cast<size_t>(n) == payload.size();
  }

  void close_all_connections() {
    std::lock_guard<std::mutex> lock(mutex_);
    close_all();
  }

private:
  static bool send_all(int fd, const std::string& payload) {
    const char* data = payload.data();
    size_t remaining = payload.size();
    size_t offset = 0;

    while (offset < remaining) {
      auto n = ::send(fd, data + offset, remaining - offset, 0);
      if (n > 0) {
        offset += static_cast<size_t>(n);
        continue;
      }
      if (n == 0) {
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
    auto it = fds_.find(endpoint);
    if (it != fds_.end()) {
      return it->second;
    }
    return connect_to(endpoint);
  }

  int connect_to(const std::string& endpoint) {
    if (endpoint.size() >= sizeof(((struct sockaddr_un*)nullptr)->sun_path)) {
      return -1;
    }

    close_fd(endpoint);

    int sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
      return -1;
    }

    if (auto flags = ::fcntl(sock, F_GETFD, 0); flags >= 0) {
      ::fcntl(sock, F_SETFD, flags | FD_CLOEXEC);
    }

    int optval = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));

    struct sockaddr_un addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, endpoint.c_str(), sizeof(addr.sun_path) - 1);

    auto addr_len = static_cast<socklen_t>(
        offsetof(struct sockaddr_un, sun_path) + endpoint.size() + 1);
    if (::connect(sock, reinterpret_cast<struct sockaddr*>(&addr), addr_len) < 0) {
      ::close(sock);
      return -1;
    }

    fds_[endpoint] = sock;
    return sock;
  }

  void close_fd(const std::string& endpoint) {
    auto it = fds_.find(endpoint);
    if (it != fds_.end()) {
      ::close(it->second);
      fds_.erase(it);
    }
  }

  void close_all() {
    for (auto& [_, fd] : fds_) {
      ::close(fd);
    }
    fds_.clear();
    for (auto& [_, fd] : dgram_fds_) {
      ::close(fd);
    }
    dgram_fds_.clear();
  }

  std::mutex mutex_;
  std::unordered_map<std::string, int> fds_;
  std::unordered_map<std::string, int> dgram_fds_;
};

} // namespace krbn
