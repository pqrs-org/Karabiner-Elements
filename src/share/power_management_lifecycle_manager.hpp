#pragma once

// `krbn::power_management_lifecycle_manager` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include "run_loop_thread_utility.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_power_management.hpp>
#include <utility>

namespace krbn {
template <typename ComponentsManager>
class power_management_lifecycle_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  using components_manager_maker = std::function<std::unique_ptr<ComponentsManager>()>;

  power_management_lifecycle_manager(const power_management_lifecycle_manager&) = delete;

  power_management_lifecycle_manager(components_manager_maker components_manager_maker,
                                     std::chrono::milliseconds system_will_sleep_delay = std::chrono::milliseconds(0))
      : dispatcher_client(),
        components_manager_maker_(std::move(components_manager_maker)),
        system_will_sleep_delay_(system_will_sleep_delay) {
    power_management_monitor_ = std::make_unique<pqrs::osx::iokit_power_management::monitor>(weak_dispatcher_,
                                                                                             run_loop_thread_utility::get_power_management_run_loop_thread());

    power_management_monitor_->system_will_sleep.connect([this](auto&& kernel_port,
                                                                auto&& notification_id,
                                                                auto&& wait) {
      logger::get_logger()->info("system_will_sleep");

      stop_components();

      auto allow_power_change = [kernel_port, notification_id, wait] {
        logger::get_logger()->info("call IOAllowPowerChange");

        IOAllowPowerChange(kernel_port, notification_id);

        wait->notify();
      };

      // Queue the acknowledgement after components_manager teardown so that
      // its IPC connections are closed before the system sleeps. Callers that
      // seize devices can specify an additional delay for asynchronous ungrab.
      if (system_will_sleep_delay_ == std::chrono::milliseconds(0)) {
        enqueue_to_dispatcher(std::move(allow_power_change));
      } else {
        enqueue_to_dispatcher(std::move(allow_power_change),
                              when_now() + system_will_sleep_delay_);
      }
    });

    power_management_monitor_->system_will_power_on.connect([this] {
      logger::get_logger()->info("system_will_power_on");

      start_components();
    });

    power_management_monitor_->system_has_powered_on.connect([this] {
      logger::get_logger()->info("system_has_powered_on");

      start_components();
    });

    power_management_monitor_->can_system_sleep.connect([](auto&& kernel_port,
                                                           auto&& notification_id,
                                                           auto&& wait) {
      logger::get_logger()->info("can_system_sleep");

      IOAllowPowerChange(kernel_port, notification_id);

      wait->notify();
    });

    power_management_monitor_->system_will_not_sleep.connect([this] {
      logger::get_logger()->info("system_will_not_sleep");

      start_components();
    });

    power_management_monitor_->error_occurred.connect([](auto&& message) {
      logger::get_logger()->error("power_management_monitor_ error: {0}", message);
    });
  }

  ~power_management_lifecycle_manager() override {
    detach_from_dispatcher([this] {
      stop_components();
      power_management_monitor_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      start_components();
      power_management_monitor_->async_start();
    });
  }

private:
  void start_components() {
    if (components_manager_) {
      return;
    }

    components_manager_ = components_manager_maker_();
    components_manager_->async_start();
  }

  void stop_components() {
    components_manager_ = nullptr;
  }

  components_manager_maker components_manager_maker_;
  std::chrono::milliseconds system_will_sleep_delay_;
  std::unique_ptr<ComponentsManager> components_manager_;
  std::unique_ptr<pqrs::osx::iokit_power_management::monitor> power_management_monitor_;
};
} // namespace krbn
