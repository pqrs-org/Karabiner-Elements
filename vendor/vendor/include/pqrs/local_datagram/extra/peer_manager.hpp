#pragma once

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../client.hpp"
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

  //
  // entry
  //

  class entry final {
  public:
    entry(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          const std::filesystem::path& peer_socket_file_path,
          size_t buffer_size)
        : client_(weak_dispatcher,
                  peer_socket_file_path,
                  std::nullopt,
                  buffer_size),
          connected_(false),
          verified_(false) {
    }

    client& get_client(void) {
      return client_;
    }

    void set_connected(bool value) {
      connected_ = value;
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
          verify_peer = [](auto&&, auto&&) { return true; },
      std::function<void(const std::filesystem::path& peer_socket_file_path)> peer_closed = [](auto&&) {})
      : dispatcher_client(weak_dispatcher),
        buffer_size_(buffer_size),
        verify_peer_(verify_peer),
        peer_closed_(peer_closed) {
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
          error(peer_socket_file_path, error_code);
        });

        it->second->get_client().closed.connect([this, weak_entry, peer_socket_file_path] {
          if (auto e = weak_entry.lock()) {
            peer_closed_(peer_socket_file_path);
          }

          entries_.erase(peer_socket_file_path);
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

private:
  size_t buffer_size_;
  std::function<bool(std::optional<pid_t> peer_pid,
                     const std::filesystem::path& peer_socket_file_path)>
      verify_peer_;
  std::function<void(const std::filesystem::path& peer_socket_file_path)> peer_closed_;

  std::unordered_map<std::filesystem::path, not_null_shared_ptr_t<entry>> entries_;
};

} // namespace extra
} // namespace local_datagram
} // namespace pqrs
