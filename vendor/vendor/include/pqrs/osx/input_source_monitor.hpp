#pragma once

// pqrs::osx::input_source_monitor v1.4.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::input_source_monitor` can be used safely in a multi-threaded environment.

// Destroy `pqrs::osx::input_source_monitor` before you call `CFRunLoopStop(CFRunLoopGetMain())`.

#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source.hpp>
#include <utility>

namespace pqrs::osx {
class input_source_monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(cf::cf_ptr<TISInputSourceRef>)> input_source_changed;

  // Methods

  input_source_monitor(const input_source_monitor&) = delete;

  explicit input_source_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher)
      : dispatcher_client(std::move(weak_dispatcher)),
        started_(false) {
  }

  ~input_source_monitor() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (started_) {
        return;
      }

      started_ = true;

      // `static_input_source_changed_callback` will be called on the main thread.
      CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                      this,
                                      static_input_source_changed_callback,
                                      kTISNotifySelectedKeyboardInputSourceChanged,
                                      nullptr,
                                      CFNotificationSuspensionBehaviorDeliverImmediately);

      // Call `input_source_changed` slots for the current input source.

      input_source_changed_callback();
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop() {
    if (!started_) {
      return;
    }

    gcd::dispatch_sync_on_main_queue(^{
      CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                         this,
                                         kTISNotifySelectedKeyboardInputSourceChanged,
                                         nullptr);
    });

    started_ = false;
    last_input_source_.reset();
  }

  // This method will be called on the main thread.
  static void static_input_source_changed_callback(CFNotificationCenterRef,
                                                   void* observer,
                                                   CFStringRef,
                                                   const void*,
                                                   CFDictionaryRef) {
    auto self = static_cast<input_source_monitor*>(observer);
    if (self) {
      self->input_source_changed_callback();
    }
  }

  // This method will be called on the main thread.
  void input_source_changed_callback() {
    enqueue_to_dispatcher([this] {
      if (auto input_source = osx::input_source::make_current_keyboard_input_source()) {
        if (cf::equal(last_input_source_, input_source)) {
          return;
        }

        last_input_source_ = input_source;

        enqueue_to_dispatcher([this, input_source] {
          input_source_changed(input_source);
        });
      }
    });
  }

  bool started_;
  cf::cf_ptr<TISInputSourceRef> last_input_source_;
};
} // namespace pqrs::osx
