#pragma once

#include "constants.hpp"
#include "grabber_client.hpp"
#include "hid_system_client.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <vector>

class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) : exit_loop_(false),
                   hid_system_client_(logger::get_logger()) {
    // Ensure that grabber is running.
    grabber_client_ = std::make_unique<grabber_client>();
    grabber_client_->connect(krbn::connect_from::event_dispatcher);

    const size_t buffer_length = 8 * 1024;
    buffer_.resize(buffer_length);

    mkdir(constants::get_socket_directory(), 0755);
    const char* path = constants::get_event_dispatcher_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);
    chmod(path, 0600);

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });

    logger::get_logger().info("receiver is started");
  }

  ~receiver(void) {
    unlink(constants::get_event_dispatcher_socket_file_path());

    exit_loop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }

    grabber_client_ = nullptr;
    server_ = nullptr;

    logger::get_logger().info("receiver is stopped");
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
        case krbn::operation_type::post_modifier_flags:
          if (n != sizeof(krbn::operation_type_post_modifier_flags_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::post_modifier_flags");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_post_modifier_flags_struct*>(&(buffer_[0]));
            if (auto mac_key = krbn::types::get_mac_key(p->key_code)) {
              hid_system_client_.post_modifier_flags(*mac_key, p->flags);
            } else {
              logger::get_logger().error("unsupported key_code {1:#x} @ {0}", __PRETTY_FUNCTION__, static_cast<uint32_t>(p->key_code));
            }
          }
          break;

        case krbn::operation_type::toggle_caps_lock_state:
          if (n != sizeof(krbn::operation_type_toggle_caps_lock_state_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::toggle_caps_lock_state");
          } else {
            auto state = hid_system_client_.get_caps_lock_state();
            if (!state) {
              logger::get_logger().error("hid_system_client_.get_caps_lock_state error @ {0}", __PRETTY_FUNCTION__);
            } else {
              if (*state) {
                hid_system_client_.set_caps_lock_state(false);
                if (grabber_client_) {
                  grabber_client_->set_caps_lock_led_state(krbn::led_state::off);
                }
              } else {
                hid_system_client_.set_caps_lock_state(true);
                if (grabber_client_) {
                  grabber_client_->set_caps_lock_led_state(krbn::led_state::on);
                }
              }
            }
          }
          break;

        case krbn::operation_type::set_caps_lock_state:
          if (n != sizeof(krbn::operation_type_set_caps_lock_state_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::set_caps_lock_state");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_set_caps_lock_state_struct*>(&(buffer_[0]));
            hid_system_client_.set_caps_lock_state(p->state);
            if (grabber_client_) {
              grabber_client_->set_caps_lock_led_state(p->state ? krbn::led_state::on : krbn::led_state::off);
            }
          }
          break;

        case krbn::operation_type::refresh_caps_lock_led:
          if (n != sizeof(krbn::operation_type_refresh_caps_lock_led_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::refresh_caps_lock_led");
          } else {
            auto state = hid_system_client_.get_caps_lock_state();
            if (!state) {
              logger::get_logger().error("hid_system_client_.get_caps_lock_state error @ {0}", __PRETTY_FUNCTION__);
            } else {
              if (grabber_client_) {
                grabber_client_->set_caps_lock_led_state(*state ? krbn::led_state::on : krbn::led_state::off);
              }
            }
          }
          break;

        case krbn::operation_type::post_key:
          if (n != sizeof(krbn::operation_type_post_key_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::post_key");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_post_key_struct*>(&(buffer_[0]));
            hid_system_client_.post_key(p->key_code, p->event_type, p->flags, p->repeat);
          }
          break;

        default:
          break;
        }
      }
    }
  }

  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::unique_ptr<grabber_client> grabber_client_;
  std::thread thread_;
  std::atomic<bool> exit_loop_;

  hid_system_client hid_system_client_;
};
