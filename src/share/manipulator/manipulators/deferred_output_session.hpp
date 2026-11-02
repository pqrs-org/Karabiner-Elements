#pragma once

#include "event_queue.hpp"
#include <functional>
#include <utility>

namespace krbn::manipulator::manipulators {

// A generated output must release all deferred modifier key_up events.
// A physical input event excludes deferred events associated with that same
// source event, so the source key itself does not immediately release the
// modifiers it generated.
enum class deferred_modifier_key_up_event_scope {
  all,
  except_front_input_event,
};

class deferred_output_session final {
public:
  struct callbacks final {
    std::function<void(const event_queue::entry&,
                       event_queue::queue&)>
        post_deferred_modifier_key_up_events;
    std::function<void(const event_queue::entry&)> cancel_to_if_alone;
  };

  explicit deferred_output_session(callbacks callbacks)
      : callbacks_(std::move(callbacks)) {
  }

  [[nodiscard]] static bool releases_deferred_modifiers(const event_queue::event& event) {
    if (auto e = event.get_if<momentary_switch_event>()) {
      // Invalid events such as `vk_none` do not produce output.
      return e->valid();
    }

    // Pointing motion does not represent an explicit key or button operation,
    // so it does not interrupt a repeating key.
    return false;
  }

  // Run a complete output operation after releasing modifiers that were kept
  // pressed solely for a previous repeating output.
  //
  // Output producers use this method as their only entry point for a new
  // output operation. The callback may add deferred key_up events to
  // `output_event_queue` before `post_events` adds the new output.
  template <typename F>
  decltype(auto) post(const event_queue::entry& front_input_event,
                      event_queue::queue& output_event_queue,
                      F&& post_events) const {
    return post(true,
                front_input_event,
                output_event_queue,
                std::forward<F>(post_events));
  }

  template <typename F>
  decltype(auto) post(bool releases_deferred_modifiers,
                      const event_queue::entry& front_input_event,
                      event_queue::queue& output_event_queue,
                      F&& post_events) const {
    if (releases_deferred_modifiers) {
      callbacks_.post_deferred_modifier_key_up_events(front_input_event,
                                                      output_event_queue);
    }

    return std::forward<F>(post_events)(output_event_queue);
  }

  void cancel_to_if_alone(const event_queue::entry& front_input_event) const {
    callbacks_.cancel_to_if_alone(front_input_event);
  }

private:
  // For example, `1 -> shift+1` leaves the generated shift pressed after the
  // initial shift+1 output so that macOS repeats shift+1 rather than 1.
  //
  // When another output is about to be posted, this callback asks the
  // manipulators participating in the pipeline up to the current output
  // queue to post any deferred modifier key_up events first. Thus, pressing
  // `b` while 1 is repeating releases the generated shift before b is
  // posted, preventing an unintended shift+b output. This also applies when
  // the repeating key and the new output are handled by different
  // manipulator managers, and to delayed outputs such as `to_if_held_down`.
  //
  // All manipulators are checked because the manipulator that owns the
  // deferred modifier key_up may not be the one producing the new output.
  //
  // `cancel_to_if_alone` synchronously notifies the same pipeline when an
  // output event was actually produced. Synchronous delivery ensures that a
  // queued source key_up cannot trigger `to_if_alone` before cancellation is
  // applied.
  callbacks callbacks_;
};

} // namespace krbn::manipulator::manipulators
