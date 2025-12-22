#pragma once

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../client.hpp"
#include <algorithm>
#include <cstdint>
#include <thread>
#include <unordered_map>

namespace pqrs {
namespace local_datagram {
namespace extra {

// Designed to manage peer clients on the server and send responses.
class peer_manager final : public dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(const std::filesystem::path& peer_socket_file_path, const std::string&)> warning;
  nod::signal<void(const std::filesystem::path& peer_socket_file_path, const asio::error_code&)> error;
  nod::signal<void(const std::filesystem::path& peer_socket_file_path, size_t remaining_verified_peer_count)> peer_closed;

  //
  // entry
  //

  class entry final {
  public:
    entry(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          const std::filesystem::path& peer_socket_file_path,
          size_t buffer_size,
          std::chrono::milliseconds server_check_interval = std::chrono::milliseconds(1000))
        : client_(weak_dispatcher,
                  peer_socket_file_path,
                  std::nullopt,
                  buffer_size),
          connected_(false),
          verified_(false) {
      client_.set_server_check_interval(server_check_interval);
    }

    client& get_client(void) {
      return client_;
    }

    void set_connected(bool value) {
      connected_ = value;
    }

    bool get_verified(void) const {
      return verified_;
    }

    void set_verified(bool value) {
      verified_ = value;
    }

    void async_send(const std::vector<uint8_t>& v) {
      flush();

      if (connected_) {
        if (verified_) {
          client_.async_send(v);
        }
      } else {
        // Since we cannot verify before the connection is established,
        // enqueue pre-connection items and evaluate them after connected.
        queue_.push_back(v);
      }
    }

    void flush(void) {
      if (connected_) {
        for (auto&& v : queue_) {
          if (verified_) {
            client_.async_send(v);
          }
        }

        queue_.clear();
      }
    }

  private:
    client client_;
    bool connected_;
    bool verified_;
    std::vector<std::vector<uint8_t>> queue_;
  };

  peer_manager(const peer_manager&) = delete;

  peer_manager(
      std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
      size_t buffer_size,
      std::function<bool(std::optional<pid_t> peer_pid,
                         const std::filesystem::path& peer_socket_file_path)>
          verify_peer = [](auto&&, auto&&) { return true; })
      : dispatcher_client(weak_dispatcher),
        buffer_size_(buffer_size),
        verify_peer_(verify_peer) {
  }

  virtual ~peer_manager(void) {
    detach_from_dispatcher([this] {
      entries_.clear();
    });
  }

  void async_send(const std::filesystem::path& peer_socket_file_path,
                  const std::vector<uint8_t>& v) {
    enqueue_to_dispatcher([this, peer_socket_file_path, v] {
      auto it = entries_.find(peer_socket_file_path);
      if (it == std::end(entries_)) {
        it = entries_.insert({peer_socket_file_path,
                              std::make_shared<entry>(weak_dispatcher_,
                                                      peer_socket_file_path,
                                                      buffer_size_)})
                 .first;

        std::weak_ptr<entry> weak_entry = make_weak(it->second);

        it->second->get_client().connected.connect([this, peer_socket_file_path, weak_entry](auto&& peer_pid) {
          if (auto e = weak_entry.lock()) {
            e->set_connected(true);
            e->set_verified(verify_peer_(peer_pid,
                                         peer_socket_file_path));
            e->flush();
          }
        });

        it->second->get_client().connect_failed.connect([this, peer_socket_file_path](auto&& error_code) {
          entries_.erase(peer_socket_file_path);
          erase_shared_secret(peer_socket_file_path);

          error(peer_socket_file_path, error_code);
        });

        it->second->get_client().closed.connect([this, weak_entry, peer_socket_file_path] {
          entries_.erase(peer_socket_file_path);
          erase_shared_secret(peer_socket_file_path);

          auto remaining_verified_peer_count = std::count_if(std::cbegin(entries_),
                                                             std::cend(entries_),
                                                             [](const auto& pair) {
                                                               return pair.second->get_verified();
                                                             });
          peer_closed(peer_socket_file_path,
                      remaining_verified_peer_count);
        });

        it->second->get_client().warning_reported.connect([this, peer_socket_file_path](auto&& message) {
          warning(peer_socket_file_path, message);
        });

        it->second->get_client().error_occurred.connect([this, peer_socket_file_path](auto&& error_code) {
          error(peer_socket_file_path, error_code);
        });

        it->second->get_client().async_start();
      }

      it->second->async_send(v);
    });
  }

  void insert_shared_secret(const std::filesystem::path& peer_socket_file_path,
                            const std::vector<uint8_t>& shared_secret) {
    std::lock_guard<std::mutex> lock(shared_secrets_mutex_);

    shared_secrets_[peer_socket_file_path] = shared_secret;
  }

  bool verify_shared_secret(const std::filesystem::path& peer_socket_file_path,
                            const std::vector<uint8_t>& shared_secret) {
    std::lock_guard<std::mutex> lock(shared_secrets_mutex_);

    auto it = shared_secrets_.find(peer_socket_file_path);
    if (it == std::end(shared_secrets_)) {
      return false;
    }

    return constant_time_equal(it->second, shared_secret);
  }

private:
  bool constant_time_equal(const std::vector<uint8_t>& a,
                           const std::vector<uint8_t>& b) const {
    if (a.size() != b.size()) {
      return false;
    }

    uint8_t diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
      diff |= static_cast<uint8_t>(a[i] ^ b[i]);
    }

    return diff == 0;
  }

  void erase_shared_secret(const std::filesystem::path& peer_socket_file_path) {
    std::lock_guard<std::mutex> lock(shared_secrets_mutex_);

    shared_secrets_.erase(peer_socket_file_path);
  }

  size_t buffer_size_;
  std::function<bool(std::optional<pid_t> peer_pid,
                     const std::filesystem::path& peer_socket_file_path)>
      verify_peer_;

  std::unordered_map<std::filesystem::path, not_null_shared_ptr_t<entry>> entries_;

  // Optional: Use this to store shared secrets.
  std::unordered_map<std::filesystem::path, std::vector<uint8_t>> shared_secrets_;
  std::mutex shared_secrets_mutex_;
};

} // namespace extra
} // namespace local_datagram
} // namespace pqrs
