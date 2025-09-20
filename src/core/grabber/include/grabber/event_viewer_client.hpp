#pragma once

// `krbn::grabber::event_viewer_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "device_grabber.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>

namespace krbn {
namespace grabber {

class event_viewer_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  event_viewer_client(const event_viewer_client&) = delete;

  event_viewer_client(const std::filesystem::path& server_socket_file_path)
      : dispatcher_client(),
        peer_verified_(false) {
    client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                             server_socket_file_path,
                                                             std::nullopt,
                                                             constants::local_datagram_buffer_size);
    client_->set_server_check_interval(std::chrono::milliseconds(3000));
    client_->set_next_heartbeat_deadline(std::chrono::milliseconds(10000));
    client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));

    client_->connected.connect([this](auto&& peer_pid) {
      if (get_shared_codesign_manager()->same_team_id(peer_pid)) {
        logger::get_logger()->info("event_viewer_client connected");
        peer_verified_ = true;
      } else {
        logger::get_logger()->error("karabiner_grabber and EventViewer are not code-signed with same Team ID");
      }
    });

    client_->connect_failed.connect([](auto&& error_code) {
      logger::get_logger()->info("event_viewer_client connect_failed");
    });

    client_->closed.connect([] {
      logger::get_logger()->info("event_viewer_client closed");
    });

    client_->error_occurred.connect([](auto&& error_code) {
      logger::get_logger()->error("event_viewer_client error_occurred: {0}", error_code.message());
    });

    client_->async_start();
  }

  virtual ~event_viewer_client(void) {
    detach_from_dispatcher([this] {
      client_ = nullptr;
    });
  }

  void async_send_manipulator_environment(const nlohmann::json& manipulator_environment_json) {
    if (peer_verified_) {
      nlohmann::json json{
          {"operation_type", operation_type::manipulator_environment},
          {"manipulator_environment", manipulator_environment_json},
      };
      client_->async_send(nlohmann::json::to_msgpack(json));
    }
  }

private:
  std::unique_ptr<pqrs::local_datagram::client> client_;
  bool peer_verified_;
};

} // namespace grabber
} // namespace krbn
