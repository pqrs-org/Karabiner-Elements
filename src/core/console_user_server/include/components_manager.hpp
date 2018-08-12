#pragma once

#include "configuration_monitor.hpp"
#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "frontmost_application_observer.hpp"
#include "grabber_client.hpp"
#include "input_source_observer.hpp"
#include "logger.hpp"
#include "menu_process_manager.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include "system_preferences_monitor.hpp"
#include "version_monitor.hpp"
#include "version_monitor_utility.hpp"
#include <thread>

namespace krbn {
class components_manager final {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) {
    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    console_user_id_monitor_.console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      version_monitor_->manual_check();

      filesystem::create_directory_with_intermediate_directories(
          constants::get_user_configuration_directory(),
          0700);

      if (uid != getuid()) {
        stop_grabber_client();
        return;
      }

      receiver_ = std::make_unique<receiver>();

      receiver_->bound.connect([this] {
        stop_grabber_client();
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

  ~components_manager(void) {
    console_user_id_monitor_.stop();

    receiver_ = nullptr;

    stop_grabber_client();
  }

private:
  void start_grabber_client(void) {
    std::lock_guard<std::mutex> lock(grabber_client_mutex_);

    if (grabber_client_) {
      return;
    }

    grabber_client_ = std::make_shared<grabber_client>();

    grabber_client_->connected.connect([this] {
      version_monitor_->manual_check();

      stop_child_components();
      start_child_components();
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->manual_check();

      stop_child_components();
    });

    grabber_client_->closed.connect([this] {
      version_monitor_->manual_check();

      stop_child_components();
    });

    grabber_client_->start();
  }

  void stop_grabber_client(void) {
    std::lock_guard<std::mutex> lock(grabber_client_mutex_);

    grabber_client_ = nullptr;
    stop_child_components();
  }

  void start_child_components(void) {
    std::lock_guard<std::mutex> lock(child_components_mutex_);

    configuration_monitor_ = std::make_shared<configuration_monitor>(
        constants::get_user_core_configuration_file_path());

    start_menu_process_manager();
    start_system_preferences_monitor();
    start_frontmost_application_observer();
    start_input_source_observer();

    configuration_monitor_->start();
  }

  void stop_child_components(void) {
    std::lock_guard<std::mutex> lock(child_components_mutex_);

    stop_menu_process_manager();
    stop_system_preferences_monitor();
    stop_frontmost_application_observer();
    stop_input_source_observer();

    configuration_monitor_ = nullptr;
  }

  void start_menu_process_manager(void) {
    if (menu_process_manager_) {
      return;
    }

    menu_process_manager_ = std::make_unique<menu_process_manager>(configuration_monitor_);
  }

  void stop_menu_process_manager(void) {
    menu_process_manager_ = nullptr;
  }

  void start_system_preferences_monitor(void) {
    if (system_preferences_monitor_) {
      return;
    }

    system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(
        [this](auto&& system_preferences) {
          std::lock_guard<std::mutex> lock(grabber_client_mutex_);

          if (grabber_client_) {
            grabber_client_->system_preferences_updated(system_preferences);
          }
        },
        configuration_monitor_);
  }

  void stop_system_preferences_monitor(void) {
    system_preferences_monitor_ = nullptr;
  }

  void start_frontmost_application_observer(void) {
    if (frontmost_application_observer_) {
      return;
    }

    frontmost_application_observer_ = std::make_unique<frontmost_application_observer>(
        [this](auto&& bundle_identifier, auto&& file_path) {
          if (bundle_identifier == "org.pqrs.Karabiner.EventViewer" ||
              bundle_identifier == "org.pqrs.Karabiner-EventViewer") {
            return;
          }

          {
            std::lock_guard<std::mutex> lock(grabber_client_mutex_);

            if (grabber_client_) {
              grabber_client_->frontmost_application_changed(bundle_identifier, file_path);
            }
          }
        });
  }

  void stop_frontmost_application_observer(void) {
    frontmost_application_observer_ = nullptr;
  }

  void start_input_source_observer(void) {
    if (input_source_observer_) {
      return;
    }

    input_source_observer_ = std::make_unique<input_source_observer>(
        [this](auto&& input_source_identifiers) {
          std::lock_guard<std::mutex> lock(grabber_client_mutex_);

          if (grabber_client_) {
            grabber_client_->input_source_changed(input_source_identifiers);
          }
        });
  }

  void stop_input_source_observer(void) {
    input_source_observer_ = nullptr;
  }

  // Core components

  std::shared_ptr<version_monitor> version_monitor_;

  console_user_id_monitor console_user_id_monitor_;
  std::unique_ptr<receiver> receiver_;

  std::shared_ptr<grabber_client> grabber_client_;
  std::mutex grabber_client_mutex_;

  // Child components

  std::mutex child_components_mutex_;

  std::shared_ptr<configuration_monitor> configuration_monitor_;
  std::unique_ptr<menu_process_manager> menu_process_manager_;
  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;
  // `frontmost_application_observer` does not work properly in karabiner_grabber after fast user switching.
  // Therefore, we have to use `frontmost_application_observer` in `console_user_server`.
  std::unique_ptr<frontmost_application_observer> frontmost_application_observer_;
  std::unique_ptr<input_source_observer> input_source_observer_;
};
} // namespace krbn
