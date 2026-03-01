#pragma once

// `krbn::event_tap_monitor` can be used safely in a multi-threaded environment.

#include "event_tap_utility.hpp"
#include "logger.hpp"
#include <chrono>
#include <deque>
#include <functional>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <unordered_map>

namespace krbn {
class event_tap_monitor final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(event_type, event_queue::event)> keyboard_event_arrived;
  nod::signal<void(event_type, event_queue::event)> pointing_device_event_arrived;

  // Methods

  event_tap_monitor(const event_tap_monitor&) = delete;

  event_tap_monitor(bool capture_keyboard_events,
                    bool manipulate_events,
                    std::function<CGEventRef _Nullable(CGEventType, CGEventRef _Nullable)> keyboard_event_manipulator = nullptr,
                    std::function<bool(CGEventType, CGEventRef _Nullable, const std::optional<std::pair<event_type, event_queue::event>>&)> should_skip_event = nullptr,
                    CGEventTapLocation tap_location = kCGHIDEventTap)
      : dispatcher_client(),
        capture_keyboard_events_(capture_keyboard_events),
        manipulate_events_(manipulate_events),
        keyboard_event_manipulator_(keyboard_event_manipulator),
        should_skip_event_(should_skip_event),
        tap_location_(tap_location),
        event_tap_(nullptr),
        run_loop_source_(nullptr) {
    cf_run_loop_thread_ = std::make_unique<pqrs::cf::run_loop_thread>(pqrs::cf::run_loop_thread::failure_policy::exit);
  }

  ~event_tap_monitor(void) {
    detach_from_dispatcher([this] {
      if (event_tap_) {
        CGEventTapEnable(event_tap_, false);

        if (run_loop_source_) {
          CFRunLoopRemoveSource(cf_run_loop_thread_->get_run_loop(),
                                run_loop_source_,
                                kCFRunLoopCommonModes);
          CFRelease(run_loop_source_);
          run_loop_source_ = nullptr;
        }

        CFRelease(event_tap_);
        event_tap_ = nullptr;
      }
      logger::get_logger()->info("event_tap_monitor terminated");
    });

    cf_run_loop_thread_->terminate();
    cf_run_loop_thread_ = nullptr;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (event_tap_) {
        logger::get_logger()->info("event_tap_monitor start is skipped (already started)");
        return;
      }

      // Observe flags changed in order to get the caps lock state and pointing device events from Apple trackpads.
      //
      // Note:
      // We cannot catch pointing device events from Apple trackpads without eventtap.
      // (Apple trackpad driver does not send events to IOKit.)

      auto mask = CGEventMaskBit(kCGEventFlagsChanged) |
                  CGEventMaskBit(kCGEventLeftMouseDown) |
                  CGEventMaskBit(kCGEventLeftMouseUp) |
                  CGEventMaskBit(kCGEventRightMouseDown) |
                  CGEventMaskBit(kCGEventRightMouseUp) |
                  CGEventMaskBit(kCGEventMouseMoved) |
                  CGEventMaskBit(kCGEventLeftMouseDragged) |
                  CGEventMaskBit(kCGEventRightMouseDragged) |
                  CGEventMaskBit(kCGEventScrollWheel) |
                  CGEventMaskBit(kCGEventOtherMouseDown) |
                  CGEventMaskBit(kCGEventOtherMouseUp) |
                  CGEventMaskBit(kCGEventOtherMouseDragged);
      if (capture_keyboard_events_) {
        mask |= CGEventMaskBit(kCGEventKeyDown) |
                CGEventMaskBit(kCGEventKeyUp);
      }

      logger::get_logger()->info("event_tap_monitor start (capture_keyboard_events={0}, manipulate_events={1}, tap_location={2})",
                                 capture_keyboard_events_,
                                 manipulate_events_,
                                 static_cast<int>(tap_location_));

      event_tap_ = CGEventTapCreate(tap_location_,
                                    kCGTailAppendEventTap,
                                    manipulate_events_ ? kCGEventTapOptionDefault : kCGEventTapOptionListenOnly,
                                    mask,
                                    event_tap_monitor::static_callback,
                                    this);

      if (event_tap_) {
        run_loop_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_, 0);
        if (run_loop_source_) {
          CFRunLoopAddSource(cf_run_loop_thread_->get_run_loop(),
                             run_loop_source_,
                             kCFRunLoopCommonModes);
          CGEventTapEnable(event_tap_, true);

          cf_run_loop_thread_->wake();

          logger::get_logger()->info("event_tap_monitor initialized");
        } else {
          logger::get_logger()->error("event_tap_monitor failed to create run_loop_source");

          CFRelease(event_tap_);
          event_tap_ = nullptr;
        }
      } else {
        logger::get_logger()->error("event_tap_monitor failed to create event tap (capture_keyboard_events={0}, manipulate_events={1})",
                                    capture_keyboard_events_,
                                    manipulate_events_);
      }
    });
  }

private:
  using fn_modified_key_down_expirations = std::deque<std::chrono::steady_clock::time_point>;

  void purge_expired_fn_modified_key_down_events(const std::chrono::steady_clock::time_point& now) {
    for (auto it = std::begin(fn_modified_key_down_event_counts_); it != std::end(fn_modified_key_down_event_counts_);) {
      auto& expirations = it->second;
      while (!expirations.empty() &&
             expirations.front() <= now) {
        expirations.pop_front();
      }

      if (expirations.empty()) {
        it = fn_modified_key_down_event_counts_.erase(it);
      } else {
        ++it;
      }
    }
  }

  static std::optional<momentary_switch_event> remap_fn_modified_key_event(const momentary_switch_event& event) {
    auto usage_pair = event.get_usage_pair();
    if (usage_pair.get_usage_page() != pqrs::hid::usage_page::keyboard_or_keypad) {
      return std::nullopt;
    }

    auto usage = usage_pair.get_usage();
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_home) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_left_arrow);
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_end) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_right_arrow);
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_page_up) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_up_arrow);
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_page_down) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_down_arrow);
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keypad_enter) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_return_or_enter);
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_forward) {
      return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_or_backspace);
    }

    return std::nullopt;
  }

  std::pair<event_type, event_queue::event> normalize_fn_modified_navigation_event(std::pair<event_type, event_queue::event> pair) {
    auto now = std::chrono::steady_clock::now();
    purge_expired_fn_modified_key_down_events(now);

    if (auto momentary_switch_event = pair.second.get_if<krbn::momentary_switch_event>()) {
      if (auto modifier_flag = momentary_switch_event->make_modifier_flag();
          modifier_flag &&
          *modifier_flag == krbn::modifier_flag::fn) {
        switch (pair.first) {
          case event_type::key_down:
            fn_pressed_ = true;
            break;

          case event_type::key_up:
            fn_pressed_ = false;
            break;

          default:
            break;
        }
      } else if (auto remapped = remap_fn_modified_key_event(*momentary_switch_event)) {
        if (pair.first == event_type::key_down) {
          if (fn_pressed_) {
            fn_modified_key_down_event_counts_[*momentary_switch_event].push_back(now + fn_modified_key_down_event_ttl_);
            pair.second = event_queue::event(*remapped);
          }
        } else if (pair.first == event_type::key_up) {
          if (auto it = fn_modified_key_down_event_counts_.find(*momentary_switch_event);
              it != std::end(fn_modified_key_down_event_counts_) &&
              !it->second.empty()) {
            it->second.pop_front();
            if (it->second.empty()) {
              fn_modified_key_down_event_counts_.erase(it);
            }
            pair.second = event_queue::event(*remapped);
          }
        }
      }
    }

    return pair;
  }

  static CGEventRef _Nullable static_callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event, void* _Nonnull refcon) {
    auto self = static_cast<event_tap_monitor*>(refcon);
    if (self) {
      return self->callback(proxy, type, event);
    }
    return nullptr;
  }

  CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event) {
    std::optional<std::pair<event_type, event_queue::event>> normalized_keyboard_event;
    switch (type) {
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
        if (auto pair = event_tap_utility::make_event(type, event)) {
          normalized_keyboard_event = normalize_fn_modified_navigation_event(*pair);
        }
        break;

      default:
        break;
    }

    switch (type) {
      case kCGEventLeftMouseDown:
      case kCGEventLeftMouseUp:
      case kCGEventRightMouseDown:
      case kCGEventRightMouseUp:
      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventScrollWheel:
      case kCGEventOtherMouseDown:
      case kCGEventOtherMouseUp:
      case kCGEventOtherMouseDragged:
        event_tap_utility::update_latest_pointing_location(event);
        break;

      default:
        break;
    }

    if (should_skip_event_ &&
        should_skip_event_(type, event, normalized_keyboard_event)) {
      return event;
    }

    switch (type) {
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        logger::get_logger()->info("Re-enable event_tap_");
        CGEventTapEnable(event_tap_, true);
        break;

      case kCGEventLeftMouseDown:
      case kCGEventLeftMouseUp:
      case kCGEventRightMouseDown:
      case kCGEventRightMouseUp:
      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventScrollWheel:
      case kCGEventOtherMouseDown:
      case kCGEventOtherMouseUp:
      case kCGEventOtherMouseDragged: {
        if (auto pair = event_tap_utility::make_event(type, event)) {
          enqueue_to_dispatcher([this, pair] {
            pointing_device_event_arrived(pair->first, pair->second);
          });
        }
        // Keep physical scroll events in the original CGEvent stream.
        // In cgeventtap mode scroll is pass-through (not re-injected by Karabiner).
        if (manipulate_events_ &&
            type != kCGEventScrollWheel) {
          return nullptr;
        }
        break;
      }

      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
        // Do not suppress auto-repeat key_down events.
        // They are generated by the system while a key is held and should pass through.
        if (type == kCGEventKeyDown &&
            event &&
            CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) != 0) {
          return event;
        }

        if (manipulate_events_ &&
            keyboard_event_manipulator_) {
          event = keyboard_event_manipulator_(type, event);
          if (!event) {
            return nullptr;
          }
        }

        if (normalized_keyboard_event) {
          if (capture_keyboard_events_) {
            enqueue_to_dispatcher([this, normalized_keyboard_event] {
              keyboard_event_arrived(normalized_keyboard_event->first, normalized_keyboard_event->second);
            });
          }
          if (manipulate_events_) {
            return nullptr;
          }
        }
        break;

      case kCGEventNull:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
        // These events are not captured.
        break;
    }

    return event;
  }

  std::unique_ptr<pqrs::cf::run_loop_thread> cf_run_loop_thread_;
  bool capture_keyboard_events_;
  bool manipulate_events_;
  std::function<CGEventRef _Nullable(CGEventType, CGEventRef _Nullable)> keyboard_event_manipulator_;
  std::function<bool(CGEventType, CGEventRef _Nullable, const std::optional<std::pair<event_type, event_queue::event>>&)> should_skip_event_;
  CGEventTapLocation tap_location_;
  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
  bool fn_pressed_{false};
  static constexpr auto fn_modified_key_down_event_ttl_ = std::chrono::seconds(30);
  std::unordered_map<momentary_switch_event, fn_modified_key_down_expirations> fn_modified_key_down_event_counts_;
};
} // namespace krbn
