#pragma once

// `krbn::session_monitor::receiver` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <vector>

namespace krbn {
namespace session_monitor {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(void)> closed;

  // Methods

  receiver(const receiver&) = delete;

  receiver(void) : dispatcher_client() {
    // We have to use `getuid` (not `geteuid`) since `karabiner_session_monitor` is run as root by suid.
    // (We have to make a socket file which includes the real user ID in the file path.)
    std::string socket_file_path(constants::get_session_monitor_receiver_socket_file_path(getuid()));

    filesystem_utility::mkdir_rootonly_directory();
    unlink(socket_file_path.c_str());

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             constants::get_local_datagram_buffer_size());
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([this] {
      logger::get_logger()->info("receiver bound");
      enqueue_to_dispatcher([this] {
        bound();
      });
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("receiver bind_failed");
      enqueue_to_dispatcher([this, error_code] {
        bind_failed(error_code);
      });
    });

    server_->closed.connect([this] {
      logger::get_logger()->info("receiver closed");
      enqueue_to_dispatcher([this] {
        closed();
      });
    });

    logger::get_logger()->info("receiver is initialized");
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
    });

    logger::get_logger()->info("receiver is terminated");
  }

  void async_start(void) {
    server_->async_start();
  }

private:
  std::unique_ptr<pqrs::local_datagram::server> server_;
};
} // namespace session_monitor
} // namespace krbn
