#pragma once

// `krbn::console_user_server::components_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "grabber_client.hpp"
#include "logger.hpp"
#include "menu_process_manager.hpp"
#include "monitor/configuration_monitor.hpp"
#include "monitor/version_monitor.hpp"
#include "receiver.hpp"
#include "updater_process_manager.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/frontmost_application_monitor.hpp>
#include <pqrs/osx/input_source_monitor.hpp>
#include <pqrs/osx/json_file_monitor.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences_monitor.hpp>
#include <thread>

namespace krbn {
namespace console_user_server {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    //
    // version_monitor_
    //

    version_monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    version_monitor_->changed.connect([](auto&& version) {
      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    });

    //
    // session_monitor_
    //

    session_monitor_ = std::make_unique<pqrs::osx::session::monitor>(weak_dispatcher_);

    session_monitor_->on_console_changed.connect([this](auto&& on_console) {
      if (!on_console) {
        receiver_ = nullptr;
        stop_grabber_client();

      } else {
        version_monitor_->async_manual_check();

        pqrs::filesystem::create_directory_with_intermediate_directories(
            constants::get_user_configuration_directory(),
            0700);

        receiver_ = nullptr;
        stop_grabber_client();

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

        receiver_->async_start();
      }
    });
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      stop_grabber_client();

      session_monitor_ = nullptr;
      receiver_ = nullptr;
      version_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();
      session_monitor_->async_start(std::chrono::milliseconds(1000));
    });
  }

private:
  void start_grabber_client(void) {
    if (grabber_client_) {
      return;
    }

    grabber_client_ = std::make_shared<grabber_client>();

    grabber_client_->connected.connect([this] {
      version_monitor_->async_manual_check();

      if (grabber_client_) {
        grabber_client_->async_connect_console_user_server();
      }

      stop_child_components();
      start_child_components();
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    grabber_client_->closed.connect([this] {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    grabber_client_->async_start();
  }

  void stop_grabber_client(void) {
    grabber_client_ = nullptr;
    stop_child_components();
  }

  void start_child_components(void) {
    configuration_monitor_ = std::make_shared<configuration_monitor>(constants::get_user_core_configuration_file_path(),
                                                                     geteuid());

    // menu_process_manager_

    menu_process_manager_ = std::make_unique<menu_process_manager>(configuration_monitor_);

    // Run NotificationWindow

    application_launcher::launch_notification_window();

    // Run MultitouchExtension

    application_launcher::launch_multitouch_extension(true);

    // updater_process_manager_

    updater_process_manager_ = std::make_unique<updater_process_manager>(configuration_monitor_);

    // system_preferences_monitor_

    system_preferences_monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(weak_dispatcher_);

    system_preferences_monitor_->system_preferences_changed.connect([this](auto&& properties_ptr) {
      if (grabber_client_) {
        grabber_client_->async_system_preferences_updated(properties_ptr);
      }
    });

    system_preferences_monitor_->async_start(std::chrono::milliseconds(3000));

    // frontmost_application_monitor_

    frontmost_application_monitor_ = std::make_unique<pqrs::osx::frontmost_application_monitor::monitor>(weak_dispatcher_);

    frontmost_application_monitor_->frontmost_application_changed.connect([this](auto&& application_ptr) {
      if (application_ptr) {
        if (application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner.EventViewer" ||
            application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner-EventViewer") {
          return;
        }

        if (grabber_client_) {
          grabber_client_->async_frontmost_application_changed(application_ptr);
        }
      }
    });

    frontmost_application_monitor_->async_start();

    // input_source_monitor_

    input_source_monitor_ = std::make_unique<pqrs::osx::input_source_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    input_source_monitor_->input_source_changed.connect([this](auto&& input_source_ptr) {
      if (input_source_ptr && grabber_client_) {
        auto properties = std::make_shared<pqrs::osx::input_source::properties>(*input_source_ptr);
        grabber_client_->async_input_source_changed(properties);
      }
    });

    input_source_monitor_->async_start();

    // Start configuration_monitor_

    configuration_monitor_->async_start();
  }

  void stop_child_components(void) {
    menu_process_manager_ = nullptr;
    updater_process_manager_ = nullptr;
    system_preferences_monitor_ = nullptr;
    frontmost_application_monitor_ = nullptr;
    input_source_monitor_ = nullptr;

    configuration_monitor_ = nullptr;
  }

  // Core components

  std::unique_ptr<version_monitor> version_monitor_;

  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;
  std::unique_ptr<receiver> receiver_;
  std::shared_ptr<grabber_client> grabber_client_;

  // Child components

  std::shared_ptr<configuration_monitor> configuration_monitor_;
  std::unique_ptr<menu_process_manager> menu_process_manager_;
  std::unique_ptr<updater_process_manager> updater_process_manager_;
  std::unique_ptr<pqrs::osx::system_preferences_monitor> system_preferences_monitor_;
  // `frontmost_application_monitor` does not work properly in karabiner_grabber after fast user switching.
  // Therefore, we have to use `frontmost_application_monitor` in `console_user_server`.
  std::unique_ptr<pqrs::osx::frontmost_application_monitor::monitor> frontmost_application_monitor_;
  std::unique_ptr<pqrs::osx::input_source_monitor> input_source_monitor_;
};
} // namespace console_user_server
} // namespace krbn
