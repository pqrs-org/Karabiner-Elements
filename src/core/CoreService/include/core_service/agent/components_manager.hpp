#pragma once

// `krbn::core_service::agent::components_manager` can be used safely in a multi-threaded environment.

#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "core_service_client.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/accessibility.hpp>

namespace krbn {
namespace core_service {
namespace agent {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client() {
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
    // accessibility_monitor_
    //

    pqrs::osx::accessibility::monitor::initialize_shared_monitor(pqrs::dispatcher::extra::get_shared_dispatcher());
    if (auto m = pqrs::osx::accessibility::monitor::get_shared_monitor().lock()) {
      m->frontmost_application_changed.connect([this](auto&& application_ptr) {
        if (core_service_client_) {
          application a;
          a.set_bundle_identifier(application_ptr->get_bundle_identifier());
          a.set_bundle_path(application_ptr->get_bundle_path());
          a.set_file_path(application_ptr->get_file_path());
          a.set_pid(application_ptr->get_pid());

          switch (application_ptr->get_detection_source()) {
            case pqrs::osx::accessibility::application::detection_source::none:
              a.set_detection_source(application::detection_source::none);
              break;

            case pqrs::osx::accessibility::application::detection_source::workspace:
              a.set_detection_source(application::detection_source::workspace);
              break;

            case pqrs::osx::accessibility::application::detection_source::ax_observer:
              a.set_detection_source(application::detection_source::ax_observer);
              break;
          }

          core_service_client_->async_frontmost_application_changed(a);
        }
      });

      m->focused_ui_element_changed.connect([this](auto&& focused_ui_element_ptr) {
        if (core_service_client_) {
          focused_ui_element e;
          e.set_role(focused_ui_element_ptr->get_role());
          e.set_subrole(focused_ui_element_ptr->get_subrole());
          e.set_title(focused_ui_element_ptr->get_title());
          e.set_window_position_x(focused_ui_element_ptr->get_window_position_x());
          e.set_window_position_y(focused_ui_element_ptr->get_window_position_y());
          e.set_window_size_width(focused_ui_element_ptr->get_window_size_width());
          e.set_window_size_height(focused_ui_element_ptr->get_window_size_height());

          core_service_client_->async_focused_ui_element_changed(e);
        }
      });
    }
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      stop_core_service_client();

      pqrs::osx::accessibility::monitor::terminate_shared_monitor();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();

      version_monitor_->async_manual_check();

      start_core_service_client();
    });
  }

private:
  void start_core_service_client(void) {
    if (core_service_client_) {
      return;
    }

    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name core_service_agent_core_service_client -> cs_agent_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/cs_agent_cs_clnt/189df82be335bb38.sock`

    core_service_client_ = std::make_shared<core_service_client>("cs_agent_cs_clnt");

    core_service_client_->connected.connect([this] {
      version_monitor_->async_manual_check();
    });

    core_service_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->async_manual_check();
    });

    core_service_client_->closed.connect([this] {
      version_monitor_->async_manual_check();
    });

    core_service_client_->async_start();
  }

  void stop_core_service_client(void) {
    core_service_client_ = nullptr;
  }

  std::unique_ptr<version_monitor> version_monitor_;
  std::shared_ptr<core_service_client> core_service_client_;
};
} // namespace agent
} // namespace core_service
} // namespace krbn
