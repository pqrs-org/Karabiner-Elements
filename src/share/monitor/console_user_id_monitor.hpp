#pragma once

// `krbn::console_user_id_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "thread_utility.hpp"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>

namespace krbn {
class console_user_id_monitor final : public pqrs::dispatcher::dispatcher_client {
public:
  // Signals

  boost::signals2::signal<void(boost::optional<uid_t>)> console_user_id_changed;

  // Methods

  console_user_id_monitor(const console_user_id_monitor&) = delete;

  console_user_id_monitor(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                                         enabled_(false) {
  }

  virtual ~console_user_id_monitor(void) {
    detach_from_dispatcher([] {
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      enabled_ = true;

      logger::get_logger().info("console_user_id_monitor is started.");

      check();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      enabled_ = false;
      uid_ = nullptr;

      logger::get_logger().info("console_user_id_monitor is stopped.");
    });
  }

private:
  void check(void) {
    if (!enabled_) {
      return;
    }

    auto u = session::get_current_console_user_id();
    if (!uid_ || *uid_ != u) {
      uid_ = std::make_unique<boost::optional<uid_t>>(u);
      console_user_id_changed(u);
    }

    enqueue_to_dispatcher(
        [this] {
          check();
        },
        when_now() + std::chrono::milliseconds(1000));
  }

  bool enabled_;
  std::unique_ptr<boost::optional<uid_t>> uid_;
};
} // namespace krbn
