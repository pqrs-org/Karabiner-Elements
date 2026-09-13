#pragma once

// `krbn::keyboard_suppression` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include "types.hpp"
#include <atomic>
#include <deque>
#include <mutex>

namespace krbn {
// keyboard_suppression keeps a short-lived set of keyboard events that should
// be ignored when they come back through the event tap.
//
// Why this is needed:
// In cgeventtap input mode, Karabiner-Elements posts transformed key events via virtual HID.
// Those posted events can be observed again by event tap.
// Without this suppression, Karabiner-Elements may treat its own posted events as new physical input
// and send them through the manipulation pipeline again.
//
// Behavior:
// - enqueue(event, event_type, now):
//   Registers a keyboard event to suppress for a short TTL window.
//   Invalid events and pointing-button events are ignored.
// - consume(event, event_type, now):
//   Returns true when a matching queued entry exists (and removes it).
//   Returns false if not found.
// - purge_expired(now):
//   Drops expired entries.
//
// Operational notes:
// - FIFO + expiration is used to avoid unbounded growth.
// - max_size bounds memory even if matching events do not arrive.
// - This class is thread-safe.
class keyboard_suppression final {
public:
  // Use 200 ms to accommodate delayed Caps Lock CGEvents.
  // macOS filters brief Caps Lock presses, so a virtual HID key_down does not
  // necessarily produce a CGEvent immediately, or at all. With CGEventTap fallback
  // enabled, we observed Caps Lock activation returning after 80-87 ms. A short TTL,
  // such as 50 ms, can expire before that event arrives, allowing our own output to enter
  // the manipulation pipeline again as device_id(0). A global caps_lock -> delete
  // mapping then generated an unwanted delete key_down and continuous deletion.
  //
  // 200 ms provides headroom above the observed delay; it is an empirical margin,
  // not a guaranteed upper bound on macOS event delivery. Keep the wait bounded:
  // brief presses may produce no CGEvent, and Caps Lock flagsChanged events encode
  // the lock state rather than physical down/up, so some entries never match.
  // A longer TTL also increases the window in which an unrelated event with the
  // same key and event type can consume a stale entry and bypass remapping.
  // Matching entries are consumed once, so this is not a key-repeat debounce.
  //
  // This changes only suppression lifetime, not the separate fallback loop guard
  // implemented in keyboard_fallback_loop_guard.hpp and called from
  // monitor/event_tap_monitor.hpp to suspend fallback processing during rapid loops.
  keyboard_suppression(std::chrono::milliseconds ttl = std::chrono::milliseconds(200),
                       size_t max_size = 1024)
      : ttl_(ttl),
        max_size_(max_size) {
  }

  void purge_expired(absolute_time_point now) {
    std::lock_guard<std::mutex> lock(mutex_);
    purge_expired_entries_unlocked(now);
  }

  void set_expired_log_enabled(bool value) {
    expired_log_enabled_.store(value);
  }

  void enqueue(const momentary_switch_event& event,
               event_type event_type,
               absolute_time_point now) {
    if (!event.valid() || event.pointing_button()) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    purge_expired_entries_unlocked(now);

    auto expires_at = now + pqrs::osx::chrono::make_absolute_time_duration(ttl_);
    entries_.push_back(entry{
        .event = event,
        .event_type = event_type,
        .expires_at = expires_at,
    });

    while (entries_.size() > max_size_) {
      entries_.pop_front();
    }
  }

  bool consume(const momentary_switch_event& event,
               event_type event_type,
               absolute_time_point now) {
    std::lock_guard<std::mutex> lock(mutex_);

    purge_expired_entries_unlocked(now);

    auto it = std::ranges::find_if(entries_,
                                   [&](const auto& entry) {
                                     return entry.event == event &&
                                            entry.event_type == event_type;
                                   });

    if (it != std::end(entries_)) {
      entries_.erase(it);
      return true;
    }

    return false;
  }

private:
  struct entry final {
    momentary_switch_event event;
    event_type event_type;
    absolute_time_point expires_at;
  };

  void purge_expired_entries_unlocked(absolute_time_point now) {
    size_t expired_count = 0;

    while (!entries_.empty() &&
           entries_.front().expires_at <= now) {
      entries_.pop_front();
      ++expired_count;
    }

    if (expired_count > 0 &&
        expired_log_enabled_.load()) {
      logger::get_logger()->warn("keyboard suppression expired before matching event ({0} entries)", expired_count);
    }
  }

  std::deque<entry> entries_;
  std::chrono::milliseconds ttl_;
  size_t max_size_;
  std::atomic<bool> expired_log_enabled_{false};
  mutable std::mutex mutex_;
};
} // namespace krbn
