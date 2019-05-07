#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "monitor/version_monitor.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/session.hpp>

namespace krbn {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(std::weak_ptr<version_monitor> weak_version_monitor) : dispatcher_client(),
                                                                            session_monitor_timer_(*this) {
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      session_monitor_timer_.stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      session_monitor_timer_.start(
          [] {
            if (auto attributes = pqrs::osx::session::find_cg_session_current_attributes()) {
              logger::get_logger()->info("cg_session_current_attributes");
              if (auto uid = attributes->get_user_id()) {
                logger::get_logger()->info("uid: {0}", *uid);
              }
              if (auto on_console = attributes->get_on_console()) {
                logger::get_logger()->info("on_console: {0}", *on_console);
              }
            } else {
              logger::get_logger()->info("cg_session_current_attributes is std::nullopt");
            }
          },
          std::chrono::milliseconds(3000));
    });
  }

private:
  pqrs::dispatcher::extra::timer session_monitor_timer_;
};
} // namespace krbn
