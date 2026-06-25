#pragma once

#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/kern_return.hpp>
#include <pqrs/thread_wait.hpp>
#include <unordered_set>
#include <vector>

namespace pqrs::osx::iokit_power_management {

namespace detail {
// Tracks one sleep-prevention response while the run loop thread is waiting for
// the dispatcher callback.
//
// If the dispatcher callback starts, the connected slot owns the final
// IOAllowPowerChange or IOCancelPowerChange decision.
// If the callback never starts because the dispatcher client is detached or the
// wait times out, this object falls back to IOAllowPowerChange and wakes the run
// loop thread.
class pending_power_response final {
public:
  using allow_power_change_function = std::function<void(io_connect_t, intptr_t)>;

  pending_power_response(
      io_connect_t kernel_port,
      intptr_t notification_id,
      allow_power_change_function allow_power_change = [](auto kernel_port, auto notification_id) {
        IOAllowPowerChange(kernel_port,
                           notification_id);
      })
      : kernel_port_(kernel_port),                          // Keep initializer list vertical.
        notification_id_(notification_id),                  // Keep initializer list vertical.
        allow_power_change_(std::move(allow_power_change)), // Keep initializer list vertical.
        wait_(make_thread_wait()) {
  }

  [[nodiscard]] not_null_shared_ptr_t<thread_wait> get_wait() const {
    return wait_;
  }

  [[nodiscard]] bool try_start_callback() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (closed_) {
      return false;
    }

    callback_started_ = true;
    return true;
  }

  // If detach_from_dispatcher removes the queued dispatcher callback before it
  // starts, call IOAllowPowerChange here instead of the connected slot.
  void allow_power_change_if_callback_not_started() {
    bool allow = false;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (!callback_started_ &&
          !closed_) {
        closed_ = true;
        allow = true;
      }
    }

    if (allow) {
      allow_power_change_(kernel_port_,
                          notification_id_);
    }
  }

  void notify_wait() {
    wait_->notify();
  }

private:
  io_connect_t kernel_port_;
  intptr_t notification_id_;
  allow_power_change_function allow_power_change_;
  not_null_shared_ptr_t<thread_wait> wait_;

  std::mutex mutex_;
  bool callback_started_ = false;
  bool closed_ = false;
};
} // namespace detail

class monitor final : dispatcher::extra::dispatcher_client {
public:
  class lifetime final {};

  static constexpr auto sleep_prevention_wait_timeout = std::chrono::seconds(30);
  using sleep_prevention_signal = nod::signal<void(io_connect_t, intptr_t, not_null_shared_ptr_t<thread_wait>)>;

  // Signals (invoked from the dispatcher thread)
  // Important: Connect or disconnect these signals before async_start.

  // wait->notify() must be called in callback.
  sleep_prevention_signal system_will_sleep;
  nod::signal<void()> system_will_power_on;
  nod::signal<void()> system_has_powered_on;
  // wait->notify() must be called in callback.
  sleep_prevention_signal can_system_sleep;
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
    release_pending_power_responses();
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
  using pending_power_response_ptr = not_null_shared_ptr_t<detail::pending_power_response>;

  void release_pending_power_responses() {
    std::vector<pending_power_response_ptr> responses;

    {
      std::lock_guard<std::mutex> lock(pending_power_responses_mutex_);

      responses.reserve(pending_power_responses_.size());
      for (const auto& response : pending_power_responses_) {
        responses.push_back(response);
      }

      pending_power_responses_.clear();
    }

    for (const auto& response : responses) {
      response->allow_power_change_if_callback_not_started();
      // Wake handle_sleep_prevention_message immediately if it is waiting in wait_notice_for.
      response->notify_wait();
    }
  }

  void handle_sleep_prevention_message(sleep_prevention_signal& signal,
                                       void* _Nullable message_argument) {
    auto notification_id = reinterpret_cast<intptr_t>(message_argument);
    auto kernel_port = kernel_port_;

    if (signal.empty()) {
      IOAllowPowerChange(kernel_port,
                         notification_id);
      return;
    }

    pending_power_response_ptr response = std::make_shared<detail::pending_power_response>(kernel_port,
                                                                                           notification_id);

    {
      std::lock_guard<std::mutex> lock(pending_power_responses_mutex_);
      pending_power_responses_.insert(response);
    }

    auto wait = response->get_wait();
    auto signal_ptr = &signal;

    if (enqueue_to_dispatcher([kernel_port, notification_id, wait, response, signal_ptr] {
          if (!response->try_start_callback()) {
            return;
          }

          (*signal_ptr)(kernel_port,
                        notification_id,
                        wait);
        })) {
      // Stop blocking the IOKit power callback after the expected response window.
      if (!wait->wait_notice_for(sleep_prevention_wait_timeout)) {
        response->allow_power_change_if_callback_not_started();
      }
    } else {
      response->allow_power_change_if_callback_not_started();
    }

    {
      std::lock_guard<std::mutex> lock(pending_power_responses_mutex_);
      pending_power_responses_.erase(response);
    }
  }

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
      case kIOMessageSystemWillSleep:
        handle_sleep_prevention_message(system_will_sleep,
                                        message_argument);
        break;

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

      case kIOMessageCanSystemSleep:
        handle_sleep_prevention_message(can_system_sleep,
                                        message_argument);
        break;

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
  std::mutex pending_power_responses_mutex_;
  std::unordered_set<pending_power_response_ptr> pending_power_responses_;

  IONotificationPortRef _Nullable notification_port_ = nullptr;
  io_connect_t kernel_port_ = 0;
  io_object_t notifier_ = IO_OBJECT_NULL;
};

} // namespace pqrs::osx::iokit_power_management
