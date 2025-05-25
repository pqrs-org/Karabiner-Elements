#pragma once

// pqrs::osx::iokit_service_monitor v6.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::iokit_service_monitor` can be used safely in a multi-threaded environment.

#include <nod/nod.hpp>
#include <optional>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_iterator.hpp>
#include <pqrs/osx/iokit_object_ptr.hpp>
#include <pqrs/osx/iokit_registry_entry.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/kern_return.hpp>
#include <unordered_set>

namespace pqrs {
namespace osx {
class iokit_service_monitor final : dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(iokit_registry_entry_id::value_t, iokit_object_ptr)> service_matched;
  nod::signal<void(iokit_registry_entry_id::value_t)> service_terminated;
  nod::signal<void(const std::string&, kern_return)> error_occurred;

  // Methods

  iokit_service_monitor(const iokit_service_monitor&) = delete;

  // CFRunLoopRun may get stuck in rare cases if cf::run_loop_thread generation is repeated frequently in macOS 13.
  // If such a condition occurs, cf::run_loop_thread detects it and calls abort to avoid it.
  // However, to avoid the problem itself, cf::run_loop_thread should be provided externally instead of having it internally.
  iokit_service_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                        std::shared_ptr<cf::run_loop_thread> run_loop_thread,
                        CFDictionaryRef _Nonnull matching_dictionary)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(run_loop_thread),
        matching_dictionary_(matching_dictionary),
        notification_port_(nullptr),
        scan_timer_(*this) {
  }

  virtual ~iokit_service_monitor(void) {
    // dispatcher_client

    detach_from_dispatcher([this] {
      scan_timer_.stop();
    });

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
    if (!notification_port_) {
      notification_port_ = IONotificationPortCreate(type_safe::get(iokit_mach_port::null));
      if (!notification_port_) {
        enqueue_to_dispatcher([this] {
          error_occurred("IONotificationPortCreate is failed.", kIOReturnError);
        });
        return;
      }

      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        run_loop_thread_->add_source(loop_source);
      } else {
        enqueue_to_dispatcher([this] {
          error_occurred("IONotificationPortGetRunLoopSource is failed.", kIOReturnError);
        });
        return;
      }
    }

    // kIOMatchedNotification

    if (!matched_notification_.get()) {
      if (*matching_dictionary_) {
        io_iterator_t it = IO_OBJECT_NULL;
        CFRetain(*matching_dictionary_);
        kern_return r = IOServiceAddMatchingNotification(notification_port_,
                                                         kIOFirstMatchNotification,
                                                         *matching_dictionary_,
                                                         &static_matched_callback,
                                                         static_cast<void*>(this),
                                                         &it);
        if (!r) {
          enqueue_to_dispatcher([this, r] {
            error_occurred("IOServiceAddMatchingNotification is failed.", r);
          });
        } else {
          matched_notification_ = iokit_iterator(it);
          IOObjectRelease(it);
          matched_callback(make_services(matched_notification_));
        }
      }
    }

    // kIOTerminatedNotification

    if (!terminated_notification_.get()) {
      if (*matching_dictionary_) {
        io_iterator_t it = IO_OBJECT_NULL;
        CFRetain(*matching_dictionary_);
        kern_return r = IOServiceAddMatchingNotification(notification_port_,
                                                         kIOTerminatedNotification,
                                                         *matching_dictionary_,
                                                         &static_terminated_callback,
                                                         static_cast<void*>(this),
                                                         &it);
        if (!r) {
          enqueue_to_dispatcher([this, r] {
            error_occurred("IOServiceAddMatchingNotification is failed.", r);
          });
        } else {
          terminated_notification_ = iokit_iterator(it);
          IOObjectRelease(it);
          terminated_callback(make_services(terminated_notification_));
        }
      }
    }

    //
    // Setup scan timer
    //

    enqueue_to_dispatcher([this] {
      registry_entry_ids_.clear();

      // There are rare cases where IOFirstMatchNotification and kIOTerminatedNotification are not triggered.
      // In such cases, service_terminated is never called for the terminated service.
      // To avoid this issue, periodic scans will be performed, and callbacks will be invoked for services that didn't receive the notification.

      scan_timer_.start(
          [this] {
            if (*matching_dictionary_) {
              io_iterator_t it = IO_OBJECT_NULL;
              CFRetain(*matching_dictionary_);
              kern_return r = IOServiceGetMatchingServices(type_safe::get(iokit_mach_port::null),
                                                           *matching_dictionary_,
                                                           &it);
              if (!r) {
                enqueue_to_dispatcher([this, r] {
                  error_occurred("IOServiceGetMatchingServices is failed.", r);
                });
              } else {
                auto services = make_services(iokit_iterator(it));
                IOObjectRelease(it);

                // Call service_matched

                for (const auto& s : services) {
                  if (auto registry_entry_id = s.find_registry_entry_id()) {
                    invoke_service_matched(*registry_entry_id, s);
                  }
                }

                // Call service_terminated

                std::unordered_set<iokit_registry_entry_id::value_t> terminated_registry_entry_ids;

                for (const auto stored_registry_entry_id : registry_entry_ids_) {
                  bool found = false;
                  for (const auto& s : services) {
                    if (auto registry_entry_id = s.find_registry_entry_id()) {
                      if (stored_registry_entry_id == *registry_entry_id) {
                        found = true;
                      }
                    }
                  }

                  if (!found) {
                    terminated_registry_entry_ids.insert(stored_registry_entry_id);
                  }
                }

                for (const auto terminated_registry_entry_id : terminated_registry_entry_ids) {
                  invoke_service_terminated(terminated_registry_entry_id);
                }
              }
            }
          },
          std::chrono::milliseconds(3000));
    });
  }

  // This method is executed in run_loop_thread_.
  void stop(void) {
    matched_notification_ = iokit_iterator();
    terminated_notification_ = iokit_iterator();

    if (notification_port_) {
      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        run_loop_thread_->remove_source(loop_source);
      }

      IONotificationPortDestroy(notification_port_);
      notification_port_ = nullptr;
    }

    enqueue_to_dispatcher([this] {
      scan_timer_.stop();
      registry_entry_ids_.clear();
    });
  }

  static void static_matched_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    auto self = static_cast<iokit_service_monitor*>(refcon);
    if (!self) {
      return;
    }

    auto services = make_services(iokit_iterator(iterator));

    self->run_loop_thread_->enqueue(^{
      self->matched_callback(services);
    });
  }

  void matched_callback(const std::vector<iokit_registry_entry>& services) {
    for (const auto& s : services) {
      if (auto registry_entry_id = s.find_registry_entry_id()) {
        enqueue_to_dispatcher([this, registry_entry_id, s] {
          invoke_service_matched(*registry_entry_id, s);
        });
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void invoke_service_matched(iokit_registry_entry_id::value_t registry_entry_id, iokit_registry_entry service) {
    if (!registry_entry_ids_.contains(registry_entry_id)) {
      registry_entry_ids_.insert(registry_entry_id);
      service_matched(registry_entry_id, service.get());
    }
  }

  static void static_terminated_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    auto self = static_cast<iokit_service_monitor*>(refcon);
    if (!self) {
      return;
    }

    auto services = make_services(iokit_iterator(iterator));

    self->run_loop_thread_->enqueue(^{
      self->terminated_callback(services);
    });
  }

  void terminated_callback(const std::vector<iokit_registry_entry>& services) {
    for (const auto& s : services) {
      if (auto registry_entry_id = s.find_registry_entry_id()) {
        enqueue_to_dispatcher([this, registry_entry_id] {
          invoke_service_terminated(*registry_entry_id);
        });
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void invoke_service_terminated(iokit_registry_entry_id::value_t registry_entry_id) {
    if (registry_entry_ids_.contains(registry_entry_id)) {
      registry_entry_ids_.erase(registry_entry_id);
      service_terminated(registry_entry_id);
    }
  }

  static std::vector<iokit_registry_entry> make_services(const iokit_iterator& iterator) {
    std::vector<iokit_registry_entry> services;

    while (true) {
      auto next = iterator.next();
      if (!next) {
        break;
      }

      services.emplace_back(iokit_registry_entry(next));
    }

    return services;
  }

  std::shared_ptr<cf::run_loop_thread> run_loop_thread_;
  cf::cf_ptr<CFDictionaryRef> matching_dictionary_;

  IONotificationPortRef _Nullable notification_port_;
  iokit_iterator matched_notification_;
  iokit_iterator terminated_notification_;

  pqrs::dispatcher::extra::timer scan_timer_;
  std::unordered_set<iokit_registry_entry_id::value_t> registry_entry_ids_;
};
} // namespace osx
} // namespace pqrs
