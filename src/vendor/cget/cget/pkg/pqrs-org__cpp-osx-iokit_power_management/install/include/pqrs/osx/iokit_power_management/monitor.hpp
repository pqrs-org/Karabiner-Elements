#pragma once

#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/kern_return.hpp>
#include <pqrs/thread_wait.hpp>

namespace pqrs {
namespace osx {
namespace iokit_power_management {

class monitor final : dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  // wait->notify() must be called in callback.
  nod::signal<void(io_connect_t, intptr_t, std::shared_ptr<thread_wait>)> system_will_sleep;
  nod::signal<void(void)> system_will_power_on;
  nod::signal<void(void)> system_has_powered_on;
  // wait->notify() must be called in callback.
  nod::signal<void(io_connect_t, intptr_t, std::shared_ptr<thread_wait>)> can_system_sleep;
  nod::signal<void(void)> system_will_not_sleep;

  nod::signal<void(const std::string&)> error_occurred;

  // Methods

  monitor(const monitor&) = delete;

  // CFRunLoopRun may get stuck in rare cases if cf::run_loop_thread generation is repeated frequently in macOS 13.
  // If such a condition occurs, cf::run_loop_thread detects it and calls abort to avoid it.
  // However, to avoid the problem itself, cf::run_loop_thread should be provided externally instead of having it internally.
  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          std::shared_ptr<cf::run_loop_thread> run_loop_thread)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(run_loop_thread),
        notification_port_(nullptr),
        kernel_port_(0),
        notifier_(IO_OBJECT_NULL) {
  }

  virtual ~monitor(void) {
    // dispatcher_client

    detach_from_dispatcher();

    // run_loop_thread

    run_loop_thread_->enqueue(^{
      stop();
    });

    // Wait until all tasks are processed

    auto wait = make_thread_wait();
    run_loop_thread_->enqueue(^{
      wait->notify();
    });
    wait->wait_notice();
  }

  void async_start(void) {
    run_loop_thread_->enqueue(^{
      start();
    });
  }

  void async_stop(void) {
    run_loop_thread_->enqueue(^{
      stop();
    });
  }

private:
  // This method is executed in run_loop_thread_.
  void start(void) {
    if (!kernel_port_) {
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
      } else {
        enqueue_to_dispatcher([this] {
          error_occurred("IONotificationPortGetRunLoopSource is failed.");
        });
        return;
      }
    }
  }

  // This method is executed in run_loop_thread_.
  void stop(void) {
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
  }

  void callback(uint32_t message_type,
                void* _Nullable message_argument) {
    switch (message_type) {
      case kIOMessageSystemWillSleep: {
        auto notification_id = reinterpret_cast<intptr_t>(message_argument);

        if (system_will_sleep.empty()) {
          IOAllowPowerChange(kernel_port_,
                             notification_id);
        } else {
          auto wait = make_thread_wait();

          enqueue_to_dispatcher([this, notification_id, wait] {
            system_will_sleep(kernel_port_,
                              notification_id,
                              wait);
          });

          wait->wait_notice();
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

        if (can_system_sleep.empty()) {
          IOAllowPowerChange(kernel_port_,
                             notification_id);
        } else {
          auto wait = make_thread_wait();

          enqueue_to_dispatcher([this, notification_id, wait] {
            can_system_sleep(kernel_port_,
                             notification_id,
                             wait);
          });

          wait->wait_notice();
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

  std::shared_ptr<cf::run_loop_thread> run_loop_thread_;

  IONotificationPortRef _Nullable notification_port_;
  io_connect_t kernel_port_;
  io_object_t notifier_;
};

} // namespace iokit_power_management
} // namespace osx
} // namespace pqrs
