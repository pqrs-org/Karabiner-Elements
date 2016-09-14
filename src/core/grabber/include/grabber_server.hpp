#pragma once

#include "codesign.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "local_datagram_server.hpp"
#include "process_monitor.hpp"
#include "session.hpp"
#include "types.hpp"
#include <vector>

class grabber_server final {
public:
  grabber_server(const grabber_server&) = delete;

  grabber_server(device_grabber& device_grabber) : device_grabber_(device_grabber),
                                                   exit_loop_(false) {
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

    if (auto uid = session::get_current_console_user_id()) {
      chown(path, *uid, 0);
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
    console_user_server_process_monitor_.reset(nullptr);
    device_grabber_.ungrab_devices();
  }

private:
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
              console_user_server_process_monitor_ = nullptr;
              console_user_server_process_monitor_ = std::make_unique<process_monitor>(logger::get_logger(),
                                                                                       p->console_user_server_pid,
                                                                                       std::bind(&grabber_server::console_user_server_exit_callback, this));
            }
          }
          break;

        case krbn::operation_type::system_preferences_values_updated:
          if (n < sizeof(krbn::operation_type_system_preferences_values_updated_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::system_preferences_values_updated ({0})", n);
          } else {
            logger::get_logger().info("system_preferences_values_updated");
          }
          break;

        case krbn::operation_type::set_caps_lock_led_state:
          if (n < sizeof(krbn::operation_type_set_caps_lock_led_state_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::set_caps_lock_led_state ({0})", n);
          } else {
            auto p = reinterpret_cast<krbn::operation_type_set_caps_lock_led_state_struct*>(&(buffer_[0]));
            // bind variables
            auto led_state = p->led_state;
            dispatch_async(dispatch_get_main_queue(), ^{
              device_grabber_.set_caps_lock_led_state(led_state);
            });
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

  void console_user_server_exit_callback(void) {
    device_grabber_.ungrab_devices();
  }

  device_grabber& device_grabber_;

  boost::optional<std::string> codesign_common_name_;

  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::thread thread_;
  std::atomic<bool> exit_loop_;

  std::unique_ptr<process_monitor> console_user_server_process_monitor_;
};
