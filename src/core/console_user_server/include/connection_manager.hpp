#pragma once

#include "configuration_manager.hpp"
#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "frontmost_application_observer.hpp"
#include "grabber_client.hpp"
#include "input_source_observer.hpp"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include "system_preferences_monitor.hpp"
#include "version_monitor.hpp"
#include <thread>

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(void) {
    // Setup grabber_client

    auto grabber_client = grabber_client::get_shared_instance();

    connections_.push_back(grabber_client->connected.connect([this] {
      version_monitor::get_shared_instance()->manual_check();

      grabber_client::get_shared_instance()->connect();

      start_child_monitors();
    }));

    connections_.push_back(grabber_client->connect_failed.connect([this](auto&& error_code) {
      version_monitor::get_shared_instance()->manual_check();

      stop_child_monitors();
    }));

    connections_.push_back(grabber_client->closed.connect([this] {
      version_monitor::get_shared_instance()->manual_check();

      stop_child_monitors();
    }));

    // Setup console_user_id_monitor_

    console_user_id_monitor_.console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      version_monitor::get_shared_instance()->manual_check();

      if (uid != getuid()) {
        stop_grabber_client();
        return;
      }

      receiver_ = std::make_unique<receiver>();

      receiver_->bound.connect([this] {
        start_grabber_client();
      });

      receiver_->bind_failed.connect([this](auto&& error_code) {
        stop_grabber_client();
      });

      receiver_->closed.connect([this] {
        stop_grabber_client();
      });

      receiver_->start();
    });
    console_user_id_monitor_.start();
  }

  ~connection_manager(void) {
    stop_grabber_client();
    console_user_id_monitor_.stop();

    for (auto&& c : connections_) {
      c.disconnect();
    }
  }

private:
  void start_grabber_client(void) {
    grabber_client::get_shared_instance()->start();
  }

  void stop_grabber_client(void) {
    stop_child_monitors();
    grabber_client::get_shared_instance()->stop();
  }

  void start_child_monitors(void) {
    start_configuration_manager();
    start_system_preferences_monitor();
    start_frontmost_application_observer();
    start_input_source_observer();
  }

  void stop_child_monitors(void) {
    stop_configuration_manager();
    stop_system_preferences_monitor();
    stop_frontmost_application_observer();
    stop_input_source_observer();
  }

  void start_configuration_manager(void) {
    std::lock_guard<std::mutex> lock(configuration_manager_mutex_);

    configuration_manager_ = nullptr;
    configuration_manager_ = std::make_unique<configuration_manager>();
  }

  void stop_configuration_manager(void) {
    std::lock_guard<std::mutex> lock(configuration_manager_mutex_);

    configuration_manager_ = nullptr;
  }

  void start_system_preferences_monitor(void) {
    std::lock_guard<std::mutex> lock(system_preferences_monitor_mutex_);

    system_preferences_monitor_ = nullptr;
    system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(
        [](auto&& system_preferences) {
          grabber_client::get_shared_instance()->system_preferences_updated(system_preferences);
        },
        configuration_manager_->get_configuration_monitor());
  }

  void stop_system_preferences_monitor(void) {
    std::lock_guard<std::mutex> lock(system_preferences_monitor_mutex_);

    system_preferences_monitor_ = nullptr;
  }

  void start_frontmost_application_observer(void) {
    std::lock_guard<std::mutex> lock(frontmost_application_observer_mutex_);

    frontmost_application_observer_ = nullptr;
    frontmost_application_observer_ = std::make_unique<frontmost_application_observer>(
        [](auto&& bundle_identifier, auto&& file_path) {
          if (bundle_identifier == "org.pqrs.Karabiner.EventViewer" ||
              bundle_identifier == "org.pqrs.Karabiner-EventViewer") {
            return;
          }

          grabber_client::get_shared_instance()->frontmost_application_changed(bundle_identifier, file_path);
        });
  }

  void stop_frontmost_application_observer(void) {
    std::lock_guard<std::mutex> lock(frontmost_application_observer_mutex_);

    frontmost_application_observer_ = nullptr;
  }

  void start_input_source_observer(void) {
    std::lock_guard<std::mutex> lock(input_source_observer_mutex_);

    input_source_observer_ = nullptr;
    input_source_observer_ = std::make_unique<input_source_observer>(
        [](auto&& input_source_identifiers) {
          grabber_client::get_shared_instance()->input_source_changed(input_source_identifiers);
        });
  }

  void stop_input_source_observer(void) {
    std::lock_guard<std::mutex> lock(input_source_observer_mutex_);

    input_source_observer_ = nullptr;
  }

  console_user_id_monitor console_user_id_monitor_;

  std::vector<boost::signals2::connection> connections_;

  std::unique_ptr<receiver> receiver_;

  std::unique_ptr<configuration_manager> configuration_manager_;
  std::mutex configuration_manager_mutex_;

  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;
  std::mutex system_preferences_monitor_mutex_;

  // `frontmost_application_observer` does not work properly in karabiner_grabber after fast user switching.
  // Therefore, we have to use `frontmost_application_observer` in `console_user_server`.
  std::unique_ptr<frontmost_application_observer> frontmost_application_observer_;
  std::mutex frontmost_application_observer_mutex_;

  std::unique_ptr<input_source_observer> input_source_observer_;
  std::mutex input_source_observer_mutex_;
};
} // namespace krbn
