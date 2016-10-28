#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <cstdlib>
#include <memory>
#include <thread>
#include <unistd.h>

namespace manipulator {
class event_dispatcher_manager final {
public:
  event_dispatcher_manager(const event_dispatcher_manager&) = delete;

  event_dispatcher_manager(void) : exit_loop_(false), pid_(0) {
    thread_ = std::thread([this] { this->worker(); });
  }

  ~event_dispatcher_manager(void) {
    exit_loop_ = true;
    if (pid_ > 0) {
      kill(pid_, SIGTERM);
    }
    thread_.join();
  }

  void relaunch(void) {
    if (pid_ > 0) {
      kill(pid_, SIGTERM);
    }
  }

  void create_event_dispatcher_client(void) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    client_ = nullptr;
    try {
      client_ = std::make_unique<local_datagram_client>(constants::get_event_dispatcher_socket_file_path());
      logger::get_logger().info("karabiner_event_dispatcher client is created.");

    } catch (...) {
      client_ = nullptr;
    }
  }

  bool is_connected(void) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    return (client_.get() != nullptr);
  }

  void set_caps_lock_state(bool state) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    if (!client_) {
      logger::get_logger().error("client_ is not ready @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    try {
      krbn::operation_type_set_caps_lock_state_struct s;
      s.state = state;
      client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
    } catch (...) {
      client_ = nullptr;
    }
  }

  void refresh_caps_lock_led(void) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    if (!client_) {
      logger::get_logger().error("client_ is not ready @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    try {
      krbn::operation_type_refresh_caps_lock_led_struct s;
      client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
    } catch (...) {
      client_ = nullptr;
    }
  }

  void post_modifier_flags(krbn::key_code key_code, IOOptionBits flags, krbn::keyboard_type keyboard_type) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    if (!client_) {
      logger::get_logger().error("client_ is not ready @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    try {
      krbn::operation_type_post_modifier_flags_struct s;
      s.key_code = key_code;
      s.flags = flags;
      s.keyboard_type = keyboard_type;
      client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
    } catch (...) {
      client_ = nullptr;
    }
  }

  void post_key(krbn::key_code key_code,
                krbn::event_type event_type,
                IOOptionBits flags,
                krbn::keyboard_type keyboard_type,
                bool repeat) {
    std::lock_guard<std::mutex> guard(client_mutex_);

    if (!client_) {
      logger::get_logger().error("client_ is not ready @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    try {
      krbn::operation_type_post_key_struct s;
      s.key_code = key_code;
      s.event_type = event_type;
      s.flags = flags;
      s.repeat = repeat;
      s.keyboard_type = keyboard_type;
      client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
    } catch (...) {
      client_ = nullptr;
    }
  }

private:
  void worker(void) {
    while (!exit_loop_) {
      // kill orphan karabiner_event_dispatcher
      system("/usr/bin/killall karabiner_event_dispatcher");

      std::this_thread::sleep_for(std::chrono::seconds(1));

      auto pid = fork();
      if (pid == 0) {
        // redirect stdout to log file
        int log_file_fd = open("/var/log/karabiner/event_dispatcher_log.txt", O_WRONLY | O_CREAT | O_TRUNC);
        if (log_file_fd >= 0) {
          dup2(log_file_fd, 1);
        }

        auto path = constants::get_event_dispatcher_binary_file_path();
        execl(path, path, nullptr);
        exit(0);
      }
      if (pid > 0) {
        logger::get_logger().info("karabiner_event_dispatcher is launched. (pid:{0})", pid);
        pid_ = pid;
        int status;
        waitpid(pid, &status, 0);
        pid_ = 0;
        logger::get_logger().info("karabiner_event_dispatcher is terminated. (pid:{0})", pid);

        // release client_
        {
          std::lock_guard<std::mutex> guard(client_mutex_);
          client_ = nullptr;
        }
      }
    }
  }

  std::thread thread_;
  std::atomic<bool> exit_loop_;

  std::unique_ptr<local_datagram_client> client_;
  std::mutex client_mutex_;

  std::atomic<pid_t> pid_;
};
}
