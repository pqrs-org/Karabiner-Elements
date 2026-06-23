#pragma once

#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/kern_return.hpp>
#include <pqrs/thread_wait.hpp>

namespace pqrs::osx::iokit_power_management {

class monitor final : dispatcher::extra::dispatcher_client {
public:
  class lifetime final {};

  static constexpr auto sleep_prevention_wait_timeout = std::chrono::seconds(30);

  // Signals (invoked from the dispatcher thread)
  // Important: Connect or disconnect these signals before async_start.

  // wait->notify() must be called in callback.
  nod::signal<void(io_connect_t, intptr_t, not_null_shared_ptr_t<thread_wait>)> system_will_sleep;
  nod::signal<void()> system_will_power_on;
  nod::signal<void()> system_has_powered_on;
  // wait->notify() must be called in callback.
  nod::signal<void(io_connect_t, intptr_t, not_null_shared_ptr_t<thread_wait>)> can_system_sleep;
  nod::signal<void()> system_will_not_sleep;

  nod::signal<void(const std::string&)> error_occurred;

  // Methods

  monitor(const monitor&) = delete;

  // Note 1:
  // Creating cf::run_loop_thread instances may rarely prevent CFRunLoop processing from starting,
  // particularly when the system is under heavy load on macOS 26.
  // In that situation, cf::run_loop_thread terminates the process to avoid hanging indefinitely.
  // If monitor owned its own cf::run_loop_thread, repeated monitor construction would also repeatedly expose that failure path.
  // For that reason, cf::run_loop_thread is injected from the outside.
  //
  // Note 2:
  // If `async_start` has been called, destroy `monitor` before terminating `run_loop_thread`.
  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          not_null_shared_ptr_t<cf::run_loop_thread> run_loop_thread)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(std::move(run_loop_thread)) {
  }

  ~monitor() override {
    // dispatcher_client

    detach_from_dispatcher();
    lifetime_.reset();

    // `registered_` covers cleanup after start() has finished.
    // `run_loop_tasks_` covers queued or running start()/stop() before registration state is updated.
    if (registered_ ||
        *run_loop_tasks_ > 0) {
      if (CFRunLoopGetCurrent() == run_loop_thread_->get_run_loop()) {
        stop();
      } else {
        not_null_shared_ptr_t<thread_wait> wait = make_thread_wait();
        run_loop_thread_->enqueue(^{
          stop();
          wait->notify();
        });
        wait->wait_notice();
      }
    }
  }

  void async_start() {
    auto run_loop_tasks = run_loop_tasks_;
    ++(*run_loop_tasks);

    auto weak_lifetime = std::weak_ptr<lifetime>(lifetime_);
    run_loop_thread_->enqueue(^{
      if (weak_lifetime.lock()) {
        start();
      }
      --(*run_loop_tasks);
    });
  }

  void async_stop() {
    auto run_loop_tasks = run_loop_tasks_;
    ++(*run_loop_tasks);

    auto weak_lifetime = std::weak_ptr<lifetime>(lifetime_);
    run_loop_thread_->enqueue(^{
      if (weak_lifetime.lock()) {
        stop();
      }
      --(*run_loop_tasks);
    });
  }

private:
  // This method is executed in run_loop_thread_.
  void start() {
    if (kernel_port_) {
      return;
    }

    kernel_port_ = IORegisterForSystemPower(
        reinterpret_cast<void*>(this),
        &notification_port_,
        [](void* refcon,
           io_service_t service,
           uint32_t message_type,
           void* message_argument) {
          auto self = reinterpret_cast<monitor*>(refcon);
          if (self) {
            self->callback(message_type,
                           message_argument);
          }
        },
        &notifier_);
    if (!kernel_port_) {
      enqueue_to_dispatcher([this] {
        error_occurred("IORegisterForSystemPower is failed.");
      });
      return;
    }

    if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
      run_loop_thread_->add_source(loop_source);
      registered_ = true;
    } else {
      cleanup_registration();

      enqueue_to_dispatcher([this] {
        error_occurred("IONotificationPortGetRunLoopSource is failed.");
      });
      return;
    }
  }

  void stop() {
    cleanup_registration();
  }

  // This method must be executed in run_loop_thread_.
  void cleanup_registration() {
    if (notifier_) {
      iokit_return r = IODeregisterForSystemPower(&notifier_);
      if (!r) {
        enqueue_to_dispatcher([this, r] {
          error_occurred(std::string("IODeregisterForSystemPower is failed: ") + r.to_string());
        });
      }

      notifier_ = IO_OBJECT_NULL;
    }

    if (kernel_port_) {
      kern_return r = IOServiceClose(kernel_port_);
      if (!r) {
        enqueue_to_dispatcher([this, r] {
          error_occurred(std::string("IOServiceClose is failed: ") + r.to_string());
        });
      }

      kernel_port_ = 0;
    }

    if (notification_port_) {
      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        run_loop_thread_->remove_source(loop_source);
      }

      IONotificationPortDestroy(notification_port_);
      notification_port_ = nullptr;
    }

    registered_ = false;
  }

  void callback(uint32_t message_type,
                void* _Nullable message_argument) {
    switch (message_type) {
      case kIOMessageSystemWillSleep: {
        auto notification_id = reinterpret_cast<intptr_t>(message_argument);
        auto kernel_port = kernel_port_;

        if (system_will_sleep.empty()) {
          IOAllowPowerChange(kernel_port,
                             notification_id);
        } else {
          not_null_shared_ptr_t<thread_wait> wait = make_thread_wait();

          if (enqueue_to_dispatcher([this, kernel_port, notification_id, wait] {
                system_will_sleep(kernel_port,
                                  notification_id,
                                  wait);
              })) {
            // Stop blocking the IOKit power callback after the expected response window.
            wait->wait_notice_for(sleep_prevention_wait_timeout);
          } else {
            IOAllowPowerChange(kernel_port,
                               notification_id);
          }
        }

        break;
      }

      case kIOMessageSystemWillPowerOn:
        enqueue_to_dispatcher([this] {
          system_will_power_on();
        });
        break;

      case kIOMessageSystemHasPoweredOn:
        enqueue_to_dispatcher([this] {
          system_has_powered_on();
        });
        break;

      case kIOMessageCanSystemSleep: {
        auto notification_id = reinterpret_cast<intptr_t>(message_argument);
        auto kernel_port = kernel_port_;

        if (can_system_sleep.empty()) {
          IOAllowPowerChange(kernel_port,
                             notification_id);
        } else {
          not_null_shared_ptr_t<thread_wait> wait = make_thread_wait();

          if (enqueue_to_dispatcher([this, kernel_port, notification_id, wait] {
                can_system_sleep(kernel_port,
                                 notification_id,
                                 wait);
              })) {
            // Stop blocking the IOKit power callback after the expected response window.
            wait->wait_notice_for(sleep_prevention_wait_timeout);
          } else {
            IOAllowPowerChange(kernel_port,
                               notification_id);
          }
        }
        break;
      }

      case kIOMessageSystemWillNotSleep:
        enqueue_to_dispatcher([this] {
          system_will_not_sleep();
        });
        break;
    }
  }

  not_null_shared_ptr_t<cf::run_loop_thread> run_loop_thread_;
  std::shared_ptr<lifetime> lifetime_ = std::make_shared<lifetime>();
  not_null_shared_ptr_t<std::atomic<std::size_t>> run_loop_tasks_ = std::make_shared<std::atomic<std::size_t>>(0);
  std::atomic<bool> registered_ = false;

  IONotificationPortRef _Nullable notification_port_ = nullptr;
  io_connect_t kernel_port_ = 0;
  io_object_t notifier_ = IO_OBJECT_NULL;
};

} // namespace pqrs::osx::iokit_power_management
