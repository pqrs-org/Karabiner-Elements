#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <asio.hpp>
#include <filesystem>
#include <thread>
#include <unordered_map>

namespace pqrs::unix_domain_stream::impl {

class client_state;
class runtime_test_access;
class server_state;

// The process-wide runtime owns the shared I/O executor and socket file path
// generations. Its single worker keeps those operations serialized and lets
// client and server destruction finish without joining an I/O thread.
class runtime final {
public:
  runtime(const runtime&) = delete;

private:
  friend class client_state;
  friend class runtime_test_access;
  friend class server_state;

  [[nodiscard]] static asio::io_context& get_io_context() {
    return get_instance().io_context_;
  }

  static void set_socket_file_path_owner(const std::filesystem::path& socket_file_path,
                                         const server_state* owner) {
    get_instance().socket_file_path_owners_[make_socket_file_path_key(socket_file_path)] = owner;
  }

  static void remove_socket_file_path(const std::filesystem::path& socket_file_path) {
    auto& owners = get_instance().socket_file_path_owners_;
    owners.erase(make_socket_file_path_key(socket_file_path));

    std::error_code error_code;
    std::filesystem::remove(socket_file_path,
                            error_code);
  }

  static void remove_socket_file_path_if_owned(const std::filesystem::path& socket_file_path,
                                               const server_state* owner) {
    auto& owners = get_instance().socket_file_path_owners_;

    if (auto it = owners.find(make_socket_file_path_key(socket_file_path));
        it != std::end(owners) &&
        it->second == owner) {
      owners.erase(it);

      std::error_code error_code;
      std::filesystem::remove(socket_file_path,
                              error_code);
    }
  }

  [[nodiscard]] static std::filesystem::path make_socket_file_path_key(const std::filesystem::path& socket_file_path) {
    std::error_code error_code;
    auto key = std::filesystem::weakly_canonical(socket_file_path,
                                                 error_code);
    if (!error_code) {
      return key;
    }

    error_code.clear();
    key = std::filesystem::absolute(socket_file_path,
                                    error_code);
    if (!error_code) {
      return key.lexically_normal();
    }

    return socket_file_path.lexically_normal();
  }

  [[nodiscard]] static runtime& get_instance() {
    static runtime instance;
    return instance;
  }

  runtime()
      : work_guard_(asio::make_work_guard(io_context_)),
        thread_([this] {
          io_context_.run();
        }) {
  }

  ~runtime() {
    work_guard_.reset();

    if (thread_.joinable()) {
      thread_.join();
    }
  }

  asio::io_context io_context_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread thread_;
  std::unordered_map<std::filesystem::path, const server_state*> socket_file_path_owners_;
};

} // namespace pqrs::unix_domain_stream::impl
