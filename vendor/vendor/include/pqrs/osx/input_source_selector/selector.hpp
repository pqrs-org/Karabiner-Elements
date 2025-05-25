#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::input_source_selector::selector` can be used safely in a multi-threaded environment.

// Destroy `pqrs::osx::input_source_selector::selector` before you call `CFRunLoopStop(CFRunLoopGetMain())`.

#include "impl/entry.hpp"
#include "specifier.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source.hpp>

namespace pqrs {
namespace osx {
namespace input_source_selector {
class selector final : public dispatcher::extra::dispatcher_client {
public:
  selector(const selector&) = delete;

  selector(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    static_enabled_input_sources_changed_callback,
                                    kTISNotifyEnabledKeyboardInputSourcesChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    enabled_input_sources_changed_callback();
  }

  virtual ~selector(void) {
    detach_from_dispatcher();

    gcd::dispatch_sync_on_main_queue(^{
      CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                         this,
                                         kTISNotifyEnabledKeyboardInputSourcesChanged,
                                         nullptr);
    });
  }

  void async_select(std::shared_ptr<std::vector<specifier>> specifiers) {
    enqueue_to_dispatcher([this, specifiers] {
      if (specifiers) {
        for (const auto& s : *specifiers) {
          for (const auto& e : entries_) {
            if (s.test(e.get_properties())) {
              e.select();
              return;
            }
          }
        }
      }
    });
  }

private:
  static void static_enabled_input_sources_changed_callback(CFNotificationCenterRef center,
                                                            void* observer,
                                                            CFStringRef notification_name,
                                                            const void* observed_object,
                                                            CFDictionaryRef user_info) {
    auto self = static_cast<selector*>(observer);
    if (self) {
      self->enabled_input_sources_changed_callback();
    }
  }

  void enabled_input_sources_changed_callback(void) {
    enqueue_to_dispatcher([this] {
      entries_.clear();

      for (const auto& input_source_ptr : osx::input_source::make_selectable_keyboard_input_sources()) {
        if (input_source_ptr) {
          entries_.emplace_back(*input_source_ptr);
        }
      }
    });
  }

  std::vector<impl::entry> entries_;
};
} // namespace input_source_selector
} // namespace osx
} // namespace pqrs
