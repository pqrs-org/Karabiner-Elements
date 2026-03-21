#pragma once

// `krbn::core_service::agent::components_manager` can be used safely in a multi-threaded environment.

#include "components_manager_killer.hpp"
#include "constants.hpp"
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
      m->frontmost_application_changed.connect([](auto&& application_ptr) {
        logger::get_logger()->info("accessibility frontmost_application_changed {}",
                                   application_ptr->get_bundle_identifier().value_or("unknown"));
      });

      m->focused_ui_element_changed.connect([](auto&& focused_ui_element_ptr) {
        logger::get_logger()->info("accessibility focused_ui_element_changed {}",
                                   focused_ui_element_ptr->get_role().value_or("unknown"));
      });
    }
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([] {
      pqrs::osx::accessibility::monitor::terminate_shared_monitor();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();

      version_monitor_->async_manual_check();
    });
  }

private:
  std::unique_ptr<version_monitor> version_monitor_;
};
} // namespace agent
} // namespace core_service
} // namespace krbn
