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
class console_user_id_monitor final : public dispatcher::dispatcher_client {
public:
  // Signals

  boost::signals2::signal<void(boost::optional<uid_t>)> console_user_id_changed;

  // Methods

  console_user_id_monitor(const console_user_id_monitor&) = delete;

  console_user_id_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
  }

  ~console_user_id_monitor(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (timer_) {
        return;
      }

      timer_ = std::make_unique<thread_utility::timer>(
          [](auto&& count) {
            if (count == 0) {
              return std::chrono::milliseconds(0);
            } else {
              return std::chrono::milliseconds(1000);
            }
          },
          thread_utility::timer::mode::repeat,
          [this] {
            enqueue_to_dispatcher([this] {
              auto u = session::get_current_console_user_id();
              if (uid_ && *uid_ == u) {
                return;
              }

              console_user_id_changed(u);
              uid_ = std::make_unique<boost::optional<uid_t>>(u);
            });
          });

      logger::get_logger().info("console_user_id_monitor is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    if (!timer_) {
      return;
    }

    timer_->cancel();
    timer_ = nullptr;

    logger::get_logger().info("console_user_id_monitor is stopped.");
  }

  std::unique_ptr<thread_utility::timer> timer_;
  std::unique_ptr<boost::optional<uid_t>> uid_;
};
} // namespace krbn
