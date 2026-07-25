#pragma once

// `krbn::process_lifecycle_manager` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include "run_loop_thread_utility.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_power_management.hpp>
#include <type_traits>
#include <utility>

namespace krbn {
class process_lifecycle_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  using termination_completion_handler_t = std::function<void()>;

  template <typename ComponentsManagerMaker>
  struct configuration {
    ComponentsManagerMaker components_manager_maker;
    termination_completion_handler_t termination_completion_handler;
    std::chrono::milliseconds system_will_sleep_delay{0};
  };

  process_lifecycle_manager(const process_lifecycle_manager&) = delete;

  ~process_lifecycle_manager() override {
    detach_from_dispatcher([this] {
      stop_components();
      acknowledge_pending_system_sleep();
      power_management_monitor_ = nullptr;
    });
  }

  template <typename ComponentsManagerMaker>
  static void initialize_shared_instance(configuration<ComponentsManagerMaker> configuration) {
    using components_manager_unique_ptr = std::invoke_result_t<ComponentsManagerMaker>;
    using maker_t = std::function<components_manager_unique_ptr()>;

    auto concrete_maker = maker_t(std::move(configuration.components_manager_maker));
    components_manager_maker maker = [concrete_maker = std::move(concrete_maker)] {
      auto manager = concrete_maker();
      if (!manager) {
        logger::get_logger()->error("components_manager_maker returned nullptr");
        return manager;
      }

      manager->async_start();
      return manager;
    };

    // Use `new` here because `std::make_shared` cannot access the private constructor.
    std::atomic_store(&instance_,
                      std::shared_ptr<process_lifecycle_manager>(
                          new process_lifecycle_manager(std::move(maker),
                                                        std::move(configuration.termination_completion_handler),
                                                        configuration.system_will_sleep_delay)));
  }

  static void terminate_shared_instance() {
    std::atomic_store(&instance_,
                      std::shared_ptr<process_lifecycle_manager>());
  }

  static void async_start() {
    if (auto manager = std::atomic_load(&instance_)) {
      manager->enqueue_to_dispatcher([manager] {
        if (manager->termination_requested_ ||
            !manager->power_management_monitor_) {
          return;
        }

        manager->start_components();
        manager->power_management_monitor_->async_start();
      });
    }
  }

  static bool async_request_termination() {
    if (auto manager = std::atomic_load(&instance_)) {
      return manager->enqueue_to_dispatcher([manager] {
        if (manager->termination_requested_) {
          return;
        }

        manager->termination_requested_ = true;

        // Destroy components and the power management monitor before stopping the main run loop so that
        // cleanup code that uses `gcd::dispatch_sync_on_main_queue` can still run
        // while the main thread is servicing the run loop.
        manager->stop_components();
        manager->acknowledge_pending_system_sleep();
        manager->power_management_monitor_ = nullptr;

        manager->termination_completion_handler_();
      });
    }

    return false;
  }

private:
  using components_manager_maker = std::function<std::unique_ptr<pqrs::dispatcher::extra::dispatcher_client>()>;

  process_lifecycle_manager(components_manager_maker make_components_manager,
                            termination_completion_handler_t termination_completion_handler,
                            std::chrono::milliseconds system_will_sleep_delay)
      : dispatcher_client(),
        components_manager_maker_(std::move(make_components_manager)),
        termination_completion_handler_(std::move(termination_completion_handler)),
        system_will_sleep_delay_(system_will_sleep_delay) {
    power_management_monitor_ = std::make_unique<pqrs::osx::iokit_power_management::monitor>(weak_dispatcher_,
                                                                                             run_loop_thread_utility::get_power_management_run_loop_thread());

    power_management_monitor_->system_will_sleep.connect([this](auto&& kernel_port,
                                                                auto&& notification_id,
                                                                auto&& wait) {
      logger::get_logger()->info("system_will_sleep");

      stop_components();

      pending_system_sleep_acknowledgement_ = [kernel_port, notification_id, wait] {
        logger::get_logger()->info("call IOAllowPowerChange");

        IOAllowPowerChange(kernel_port, notification_id);

        wait->notify();
      };

      if (!enqueue_to_dispatcher(
              [this] {
                acknowledge_pending_system_sleep();
              },
              when_now() + system_will_sleep_delay_)) {
        acknowledge_pending_system_sleep();
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

  void start_components() {
    if (components_manager_ || termination_requested_) {
      return;
    }

    components_manager_ = components_manager_maker_();
  }

  void stop_components() {
    components_manager_ = nullptr;
  }

  void acknowledge_pending_system_sleep() {
    auto acknowledgement = std::move(pending_system_sleep_acknowledgement_);
    pending_system_sleep_acknowledgement_ = nullptr;

    if (acknowledgement) {
      acknowledgement();
    }
  }

  inline static std::shared_ptr<process_lifecycle_manager> instance_;

  components_manager_maker components_manager_maker_;
  std::unique_ptr<pqrs::dispatcher::extra::dispatcher_client> components_manager_;
  std::unique_ptr<pqrs::osx::iokit_power_management::monitor> power_management_monitor_;
  termination_completion_handler_t termination_completion_handler_;
  std::chrono::milliseconds system_will_sleep_delay_;
  std::function<void()> pending_system_sleep_acknowledgement_;
  bool termination_requested_{false};
};
} // namespace krbn
