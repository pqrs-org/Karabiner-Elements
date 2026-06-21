#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::input_source_selector::selector` can be used safely in a multi-threaded environment.

// Destroy `pqrs::osx::input_source_selector::selector` before you call `CFRunLoopStop(CFRunLoopGetMain())`.

#include "impl/entry.hpp"
#include "specifier.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source.hpp>
#include <utility>
#include <vector>

namespace pqrs::osx::input_source_selector {
class selector final : public dispatcher::extra::dispatcher_client {
public:
  selector(const selector&) = delete;
  selector& operator=(const selector&) = delete;

  explicit selector(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher)
      : dispatcher_client(std::move(weak_dispatcher)) {
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    static_enabled_input_sources_changed_callback,
                                    kTISNotifyEnabledKeyboardInputSourcesChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    enabled_input_sources_changed_callback();
  }

  ~selector() override {
    detach_from_dispatcher();

    gcd::dispatch_sync_on_main_queue(^{
      CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                         this,
                                         kTISNotifyEnabledKeyboardInputSourcesChanged,
                                         nullptr);
    });
  }

  void async_select(std::vector<specifier> specifiers) {
    enqueue_to_dispatcher([this, specifiers = std::move(specifiers)] {
      for (const auto& s : specifiers) {
        for (const auto& e : entries_) {
          if (s.test(e.get_properties())) {
            e.select();
            return;
          }
        }
      }
    });
  }

private:
  static void static_enabled_input_sources_changed_callback(CFNotificationCenterRef,
                                                            void* observer,
                                                            CFStringRef,
                                                            const void*,
                                                            CFDictionaryRef) {
    if (auto self = static_cast<selector*>(observer)) {
      self->enabled_input_sources_changed_callback();
    }
  }

  void enabled_input_sources_changed_callback() {
    enqueue_to_dispatcher([this] {
      auto input_source_ptrs = osx::input_source::make_selectable_keyboard_input_sources();

      std::vector<impl::entry> entries;
      entries.reserve(input_source_ptrs.size());

      for (auto& input_source_ptr : input_source_ptrs) {
        if (input_source_ptr) {
          entries.emplace_back(std::move(input_source_ptr));
        }
      }

      entries_ = std::move(entries);
    });
  }

  std::vector<impl::entry> entries_;
};
} // namespace pqrs::osx::input_source_selector
