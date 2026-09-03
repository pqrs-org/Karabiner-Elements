#pragma once

// `krbn::console_user_server::update_check_scheduler` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include "update_utility.hpp"
#include <chrono>
#include <ctime>
#include <pqrs/dispatcher.hpp>
#include <random>

namespace krbn::console_user_server {
class update_check_scheduler final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  update_check_scheduler(const update_check_scheduler&) = delete;

  update_check_scheduler()
      : dispatcher_client(),
        check_for_updates_task_(*this) {
  }

  ~update_check_scheduler() override {
    detach_from_dispatcher([this] {
      check_for_updates_task_.cancel();
    });
  }

  void async_set_enabled(bool enabled) {
    enqueue_to_dispatcher([this, enabled] {
      if (check_for_updates_enabled_ == enabled) {
        return;
      }

      check_for_updates_enabled_ = enabled;

      if (enabled) {
        logger::get_logger()->info("Check for updates is enabled; waiting 30 seconds before checking for updates.");

        // Note:
        //
        // During the updates, Karabiner-Updater.app and console_user_server binaries are overwritten asynchronous.
        // And console_user_server will be restarted via version check.
        // If console_user_server is restarted before Karabiner-Updater.app overwritten,
        // checking for updates runs with the old version of Karabiner-Updater.app,
        // and the update dialog is shown again even though the update was just completed.
        //
        // Wait before checking for updates to avoid it.

        schedule_check_for_updates(std::chrono::seconds(30));
      } else {
        check_for_updates_task_.cancel();
      }
    });
  }

private:
  void schedule_check_for_updates(std::chrono::milliseconds delay) {
    auto next_check_time = when_now() + delay;

    if (check_for_updates_task_.debounce_at(
            [this] {
              if (!check_for_updates_enabled_) {
                return;
              }

              logger::get_logger()->info("Check for updates...");
              update_utility::check_for_updates_in_background();

              schedule_check_for_updates(make_next_check_for_updates_interval());
            },
            next_check_time)) {
      auto time = std::chrono::system_clock::to_time_t(next_check_time);
      std::tm local_time{};
      localtime_r(&time, &local_time);

      logger::get_logger()->info(
          "Next update check: {0:04d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d}",
          local_time.tm_year + 1900,
          local_time.tm_mon + 1,
          local_time.tm_mday,
          local_time.tm_hour,
          local_time.tm_min,
          local_time.tm_sec);
    }
  }

  static std::chrono::milliseconds make_next_check_for_updates_interval() {
    static std::random_device random_device;
    static std::mt19937 engine(random_device());
    static std::uniform_int_distribution<int> jitter_minutes(0, 60);

    return std::chrono::hours(24) +
           std::chrono::minutes(jitter_minutes(engine));
  }

  pqrs::dispatcher::extra::debounced_task check_for_updates_task_;
  bool check_for_updates_enabled_ = false;
};
} // namespace krbn::console_user_server
