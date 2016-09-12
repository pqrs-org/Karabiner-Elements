#pragma once

#include "codesign.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "device_grabber.hpp"
#include "local_datagram_server.hpp"
#include "session.hpp"
#include "types.hpp"
#include <vector>

class grabber_server final {
public:
  grabber_server(const grabber_server&) = delete;

  grabber_server(device_grabber& device_grabber) : device_grabber_(device_grabber),
                                                   exit_loop_(false),
                                                   grabber_client_pid_monitor_(0) {
    codesign_common_name_ = codesign::get_common_name_of_process(getpid());
    if (codesign_common_name_) {
      logger::get_logger().info("This process is signed with {1} @ {0}", __PRETTY_FUNCTION__, *codesign_common_name_);
    } else {
      logger::get_logger().warn("This process is not signed.");
    }

    const size_t buffer_length = 1024 * 1024;
    buffer_.resize(buffer_length);
  }

  void start(void) {
    const char* path = constants::get_grabber_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    uid_t uid;
    if (session::get_current_console_user_id(uid)) {
      chown(path, uid, 0);
    }
    chmod(path, 0600);

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });
  }

  void stop(void) {
    unlink(constants::get_grabber_socket_file_path());

    exit_loop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }

    server_.reset(nullptr);
    release_grabber_client_pid_monitor();
    device_grabber_.ungrab_devices();
  }

  void worker(void) {
    if (!server_) {
      return;
    }

    while (!exit_loop_) {
      boost::system::error_code ec;
      std::size_t n = server_->receive(boost::asio::buffer(buffer_), boost::posix_time::seconds(1), ec);

      if (!ec && n > 0) {
        switch (krbn::operation_type(buffer_[0])) {
        case krbn::operation_type::connect:
          if (n != sizeof(krbn::operation_type_connect_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::connect");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_connect_struct*>(&(buffer_[0]));
            auto pid = p->console_user_server_pid;

            bool verified_client = false;

            if (!codesign_common_name_) {
              verified_client = true;
            } else {
              auto client_common_name = codesign::get_common_name_of_process(pid);
              if (client_common_name) {
                logger::get_logger().info("A client is signed with {1} @ {0}", __PRETTY_FUNCTION__, *client_common_name);
                if (*codesign_common_name_ == *client_common_name) {
                  verified_client = true;
                }
              } else {
                logger::get_logger().warn("A client is not signed.");
              }
            }

            if (!verified_client) {
              logger::get_logger().error("A unverified client was rejected.");
            } else {
              logger::get_logger().info("grabber_client is connected (pid:{0})", pid);

              device_grabber_.post_connect_ack();

              device_grabber_.grab_devices();

              // monitor the last process
              release_grabber_client_pid_monitor();
              grabber_client_pid_monitor_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC,
                                                                   p->console_user_server_pid,
                                                                   DISPATCH_PROC_EXIT,
                                                                   dispatch_get_main_queue());
              if (!grabber_client_pid_monitor_) {
                logger::get_logger().error("{0}: failed to dispatch_source_create", __PRETTY_FUNCTION__);
              } else {
                dispatch_source_set_event_handler(grabber_client_pid_monitor_, ^{
                  logger::get_logger().info("grabber_client is exited (pid:{0})", pid);

                  device_grabber_.ungrab_devices();
                  release_grabber_client_pid_monitor();
                });
                dispatch_resume(grabber_client_pid_monitor_);
              }
            }
          }
          break;

        case krbn::operation_type::clear_simple_modifications:
          device_grabber_.clear_simple_modifications();
          break;

        case krbn::operation_type::add_simple_modification:
          if (n < sizeof(krbn::operation_type_add_simple_modification_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::add_simple_modification ({0})", n);
          } else {
            auto p = reinterpret_cast<krbn::operation_type_add_simple_modification_struct*>(&(buffer_[0]));
            device_grabber_.add_simple_modification(p->from_key_code, p->to_key_code);
          }
          break;

        default:
          break;
        }
      }
    }
  }

private:
  void release_grabber_client_pid_monitor(void) {
    if (grabber_client_pid_monitor_) {
      dispatch_source_cancel(grabber_client_pid_monitor_);
      dispatch_release(grabber_client_pid_monitor_);
      grabber_client_pid_monitor_ = 0;
    }
  }

  device_grabber& device_grabber_;

  boost::optional<std::string> codesign_common_name_;

  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::thread thread_;
  volatile bool exit_loop_;

  dispatch_source_t grabber_client_pid_monitor_;
};
