#pragma once

// `krbn::event_tap_monitor` can be used safely in a multi-threaded environment.

#include "event_tap_utility.hpp"
#include "keyboard_suppression.hpp"
#include "logger.hpp"
#include "pressed_keys_manager.hpp"
#include <chrono>
#include <deque>
#include <nod/nod.hpp>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/chrono.hpp>
#include <unordered_map>

namespace krbn {
class event_tap_monitor final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(event_type, event_queue::event)> keyboard_event_arrived;
  nod::signal<void(event_type, event_queue::event)> pointing_device_event_arrived;

  // Methods

  event_tap_monitor(const event_tap_monitor&) = delete;

  event_tap_monitor(bool cgeventtap_fallback_enabled,
                    pqrs::not_null_shared_ptr_t<pressed_keys_manager> virtual_hid_keyboard_pressed_keys_manager,
                    pqrs::not_null_shared_ptr_t<keyboard_suppression> keyboard_suppression)
      : dispatcher_client(),
        cgeventtap_fallback_enabled_(cgeventtap_fallback_enabled),
        virtual_hid_keyboard_pressed_keys_manager_(virtual_hid_keyboard_pressed_keys_manager),
        keyboard_suppression_(keyboard_suppression) {
    cf_run_loop_thread_ = std::make_unique<pqrs::cf::run_loop_thread>(pqrs::cf::run_loop_thread::failure_policy::exit);
  }

  ~event_tap_monitor(void) {
    detach_from_dispatcher([this] {
      if (event_tap_) {
        enable_event_tap(false, "terminate");

        if (run_loop_source_) {
          CFRunLoopRemoveSource(cf_run_loop_thread_->get_run_loop(),
                                run_loop_source_.get(),
                                kCFRunLoopCommonModes);
          run_loop_source_ = nullptr;
        }

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

      // Observe pointing device events from Apple trackpads.
      //
      // Note:
      // We cannot catch pointing device events from Apple trackpads without eventtap.
      // (Apple trackpad driver does not send events to IOKit.)

      auto mask = CGEventMaskBit(kCGEventLeftMouseDown) |
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
      if (cgeventtap_fallback_enabled_) {
        mask |= CGEventMaskBit(kCGEventKeyDown) |
                CGEventMaskBit(kCGEventKeyUp) |
                CGEventMaskBit(kCGEventFlagsChanged) |
                CGEventMaskBit(static_cast<CGEventType>(NX_SYSDEFINED));
      }

      logger::get_logger()->info("event_tap_monitor start (enable_cgeventtap_fallback={0})",
                                 cgeventtap_fallback_enabled_);

      auto event_tap = CGEventTapCreate(kCGHIDEventTap,
                                        kCGTailAppendEventTap,
                                        cgeventtap_fallback_enabled_ ? kCGEventTapOptionDefault : kCGEventTapOptionListenOnly,
                                        mask,
                                        event_tap_monitor::static_callback,
                                        this);
      event_tap_ = event_tap;
      if (event_tap) {
        CFRelease(event_tap);
      }

      if (event_tap_) {
        auto run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_.get(), 0);
        run_loop_source_ = run_loop_source;
        if (run_loop_source) {
          CFRelease(run_loop_source);
        }

        if (run_loop_source_) {
          CFRunLoopAddSource(cf_run_loop_thread_->get_run_loop(),
                             run_loop_source_.get(),
                             kCFRunLoopCommonModes);
          enable_event_tap(true, "async_start");

          cf_run_loop_thread_->wake();

          logger::get_logger()->info("event_tap_monitor initialized");
        } else {
          logger::get_logger()->error("event_tap_monitor failed to create run_loop_source");

          event_tap_ = nullptr;
        }
      } else {
        logger::get_logger()->error("event_tap_monitor failed to create event tap (enable_cgeventtap_fallback={0})",
                                    cgeventtap_fallback_enabled_);
      }
    });
  }

private:
  using fn_modified_key_down_expirations = std::deque<std::chrono::steady_clock::time_point>;
  static constexpr CGEventFlags modifier_event_flags_mask =
      kCGEventFlagMaskShift |
      kCGEventFlagMaskControl |
      kCGEventFlagMaskAlternate |
      kCGEventFlagMaskCommand |
      kCGEventFlagMaskAlphaShift |
      kCGEventFlagMaskSecondaryFn;

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

  CGEventFlags make_virtual_hid_modifier_flags(void) const {
    CGEventFlags flags = 0;

    for (const auto& event : virtual_hid_keyboard_pressed_keys_manager_->make_entries()) {
      if (auto modifier = event.make_modifier_flag()) {
        switch (*modifier) {
          case modifier_flag::left_shift:
          case modifier_flag::right_shift:
            flags |= kCGEventFlagMaskShift;
            break;

          case modifier_flag::left_control:
          case modifier_flag::right_control:
            flags |= kCGEventFlagMaskControl;
            break;

          case modifier_flag::left_option:
          case modifier_flag::right_option:
            flags |= kCGEventFlagMaskAlternate;
            break;

          case modifier_flag::left_command:
          case modifier_flag::right_command:
            flags |= kCGEventFlagMaskCommand;
            break;

          case modifier_flag::caps_lock:
            flags |= kCGEventFlagMaskAlphaShift;
            break;

          case modifier_flag::fn:
            flags |= kCGEventFlagMaskSecondaryFn;
            break;

          case modifier_flag::zero:
          case modifier_flag::end_:
            break;
        }
      }
    }

    return flags;
  }

  void sync_pointing_event_modifier_flags(CGEventRef _Nullable event) const {
    if (!cgeventtap_fallback_enabled_ ||
        !event) {
      return;
    }

    auto current_flags = CGEventGetFlags(event);
    auto virtual_hid_modifier_flags = make_virtual_hid_modifier_flags();
    auto synchronized_flags =
        (current_flags & ~modifier_event_flags_mask) |
        (virtual_hid_modifier_flags & modifier_event_flags_mask);
    CGEventSetFlags(event, synchronized_flags);
  }

  void enable_event_tap(bool enable, std::string_view context) const {
    if (!event_tap_) {
      return;
    }

    CGEventTapEnable(event_tap_.get(), enable);

    if (CGEventTapIsEnabled(event_tap_.get()) != enable) {
      logger::get_logger()->error("CGEventTapEnable failed ({0}, enable={1})", context, enable);
    }
  }

  CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event) {
    switch (type) {
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        logger::get_logger()->info("Re-enable event_tap_");
        enable_event_tap(true, "callback");
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
      case kCGEventOtherMouseDragged:
        // In CGEventTap fallback mode, physical modifier states may remain in mouse event flags
        // even if keyboard events are remapped.
        // (This happens when the CGEventTap fallback is enabled and the keyboard is ignored by settings.)
        // Example: left_control is remapped to left_shift, but a click is still interpreted as a control-click.
        // Align pointing-event modifier flags with the modifiers currently pressed on the virtual HID device.
        sync_pointing_event_modifier_flags(event);
        if (auto pair = event_tap_utility::make_event(type, event)) {
          enqueue_to_dispatcher([this, pair] {
            pointing_device_event_arrived(pair->first, pair->second);
          });
        }
        break;

      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
      case static_cast<CGEventType>(NX_SYSDEFINED): {
        if (auto pair = event_tap_utility::make_event(type, event)) {
          auto normalized_keyboard_event = normalize_fn_modified_navigation_event(*pair);

          if (should_skip_keyboard_event(type, event, normalized_keyboard_event)) {
            return event;
          }

          if (cgeventtap_fallback_enabled_) {
            enqueue_to_dispatcher([this, normalized_keyboard_event] {
              keyboard_event_arrived(normalized_keyboard_event.first, normalized_keyboard_event.second);
            });

            return nullptr;
          }
        }
        break;
      }

      case kCGEventNull:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
        // These events are not captured.
        break;
    }

    return event;
  }

  bool should_skip_keyboard_event(CGEventType type,
                                  CGEventRef _Nullable event,
                                  std::pair<event_type, event_queue::event>& normalized_keyboard_event) {
    if (!cgeventtap_fallback_enabled_ ||
        !event) {
      return false;
    }

    // Pass through auto-repeat only if the same key is being held by virtual HID.
    // (e.g., do not pass through physical escape repeat when escape is remapped to left_shift.)
    if (type == kCGEventKeyDown &&
        CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) != 0) {
      if (auto m = normalized_keyboard_event.second.template get_if<momentary_switch_event>()) {
        return virtual_hid_keyboard_pressed_keys_manager_->contains(*m);
      }

      return true;
    }

    auto now = pqrs::osx::chrono::mach_absolute_time_point();

    // Skip keyboard events emitted from virtual HID so they bypass Karabiner
    // processing and pass through to apps.
    if (auto m = normalized_keyboard_event.second.template get_if<momentary_switch_event>()) {
      return keyboard_suppression_->consume(*m,
                                            normalized_keyboard_event.first,
                                            now);
    }

    return false;
  }

  std::unique_ptr<pqrs::cf::run_loop_thread> cf_run_loop_thread_;
  bool cgeventtap_fallback_enabled_;
  pqrs::not_null_shared_ptr_t<pressed_keys_manager> virtual_hid_keyboard_pressed_keys_manager_;
  pqrs::not_null_shared_ptr_t<keyboard_suppression> keyboard_suppression_;
  pqrs::cf::cf_ptr<CFMachPortRef> event_tap_;
  pqrs::cf::cf_ptr<CFRunLoopSourceRef> run_loop_source_;
  bool fn_pressed_{false};
  static constexpr auto fn_modified_key_down_event_ttl_ = std::chrono::seconds(30);
  std::unordered_map<momentary_switch_event, fn_modified_key_down_expirations> fn_modified_key_down_event_counts_;
};
} // namespace krbn
