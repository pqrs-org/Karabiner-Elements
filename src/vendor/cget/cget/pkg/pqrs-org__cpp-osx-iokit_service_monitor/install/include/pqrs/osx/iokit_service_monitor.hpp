#pragma once

// pqrs::osx::iokit_service_monitor v4.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

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

  iokit_service_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                        CFDictionaryRef _Nonnull matching_dictionary) : dispatcher_client(weak_dispatcher),
                                                                        matching_dictionary_(matching_dictionary),
                                                                        notification_port_(nullptr) {
    cf_run_loop_thread_ = std::make_unique<cf::run_loop_thread>();
  }

  virtual ~iokit_service_monitor(void) {
    // dispatcher_client

    detach_from_dispatcher();

    // cf_run_loop_thread

    cf_run_loop_thread_->enqueue(^{
      stop();
    });

    cf_run_loop_thread_->terminate();
    cf_run_loop_thread_ = nullptr;
  }

  void async_start(void) {
    cf_run_loop_thread_->enqueue(^{
      start();
    });
  }

  void async_stop(void) {
    cf_run_loop_thread_->enqueue(^{
      stop();
    });
  }

  void async_invoke_service_matched(void) {
    cf_run_loop_thread_->enqueue(^{
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
          matched_callback(make_services(iokit_iterator(it)));
          IOObjectRelease(it);
        }
      }
    });
  }

private:
  // This method is executed in cf_run_loop_thread_.
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
        CFRunLoopAddSource(cf_run_loop_thread_->get_run_loop(),
                           loop_source,
                           kCFRunLoopCommonModes);
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
  }

  // This method is executed in cf_run_loop_thread_.
  void stop(void) {
    matched_notification_ = iokit_iterator();
    terminated_notification_ = iokit_iterator();

    if (notification_port_) {
      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        CFRunLoopRemoveSource(cf_run_loop_thread_->get_run_loop(),
                              loop_source,
                              kCFRunLoopCommonModes);
      }

      IONotificationPortDestroy(notification_port_);
      notification_port_ = nullptr;
    }
  }

  static void static_matched_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    auto self = static_cast<iokit_service_monitor*>(refcon);
    if (!self) {
      return;
    }

    auto services = make_services(iokit_iterator(iterator));

    self->cf_run_loop_thread_->enqueue(^{
      self->matched_callback(services);
    });
  }

  void matched_callback(const std::vector<iokit_registry_entry>& services) const {
    for (const auto& s : services) {
      if (auto registry_entry_id = s.find_registry_entry_id()) {
        enqueue_to_dispatcher([this, registry_entry_id, s] {
          service_matched(*registry_entry_id, s.get());
        });
      }
    }
  }

  static void static_terminated_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    auto self = static_cast<iokit_service_monitor*>(refcon);
    if (!self) {
      return;
    }

    auto services = make_services(iokit_iterator(iterator));

    self->cf_run_loop_thread_->enqueue(^{
      self->terminated_callback(services);
    });
  }

  void terminated_callback(const std::vector<iokit_registry_entry>& services) const {
    for (const auto& s : services) {
      if (auto registry_entry_id = s.find_registry_entry_id()) {
        enqueue_to_dispatcher([this, registry_entry_id] {
          service_terminated(*registry_entry_id);
        });
      }
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

  cf::cf_ptr<CFDictionaryRef> matching_dictionary_;

  std::unique_ptr<cf::run_loop_thread> cf_run_loop_thread_;
  IONotificationPortRef _Nullable notification_port_;
  iokit_iterator matched_notification_;
  iokit_iterator terminated_notification_;
};
} // namespace osx
} // namespace pqrs
