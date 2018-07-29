#pragma once

#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "device_observer.hpp"
#include "gcd_utility.hpp"
#include "grabber_client.hpp"
#include "version_monitor.hpp"

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(void) : client_available_states_(static_cast<size_t>(client_type::end_), false) {
    // Setup grabber_client

    auto grabber_client = grabber_client::get_shared_instance();

    connections_.push_back(grabber_client->connected.connect([this] {
      version_monitor::get_shared_instance()->manual_check();

      set_client_available_state(client_type::grabber_client, true);
    }));

    connections_.push_back(grabber_client->connect_failed.connect([this](auto&& error_code) {
      version_monitor::get_shared_instance()->manual_check();

      set_client_available_state(client_type::grabber_client, false);
    }));

    connections_.push_back(grabber_client->closed.connect([this] {
      version_monitor::get_shared_instance()->manual_check();

      set_client_available_state(client_type::grabber_client, false);
    }));

    grabber_client->start();
  }

  ~connection_manager(void) {
    for (auto&& c : connections_) {
      c.disconnect();
    }
  }

private:
  enum class client_type {
    grabber_client,
    end_,
  };

  void set_client_available_state(client_type type, bool value) {
    {
      std::lock_guard<std::mutex> lock(client_available_states_mutex_);

      client_available_states_[static_cast<size_t>(type)] = value;
    }

    update_device_observer();
  }

  void update_device_observer(void) {
    bool device_observer_available = false;

    {
      std::lock_guard<std::mutex> lock(client_available_states_mutex_);

      if (std::all_of(std::begin(client_available_states_),
                      std::end(client_available_states_),
                      [](auto&& state) { return state == true; })) {
        device_observer_available = true;
      }
    }

    // Update device_observer_.

    {
      std::lock_guard<std::mutex> lock(device_observer_mutex_);

      if (device_observer_available) {
        if (!device_observer_) {
          device_observer_ = std::make_unique<device_observer>();
          logger::get_logger().info("device_observer is created.");
        }
      } else {
        if (device_observer_) {
          device_observer_ = nullptr;
          logger::get_logger().info("device_observer is destroyed.");
        }
      }
    }
  }

  std::vector<boost::signals2::connection> connections_;

  std::vector<bool> client_available_states_;
  std::mutex client_available_states_mutex_;

  std::unique_ptr<device_observer> device_observer_;
  std::mutex device_observer_mutex_;
};
} // namespace krbn
