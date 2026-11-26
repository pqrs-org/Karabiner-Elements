#pragma once

#include "event_queue.hpp"

namespace krbn::manipulator::manipulators::basic::manipulated_original_event {
class events_at_key_up final {
public:
  class entry {
  public:
    struct parameters final {
      krbn::device_id device_id;
      const event_queue::event& event;
      krbn::event_type event_type;
      const event_queue::event& original_event;

      // Controls HID dispatch after the release has been posted to the event queue.
      // A lazy modifier change updates the queue state immediately. The virtual device sees
      // the resulting state at the next sync, such as before the next non-modifier key_down.
      // The same mechanism is used for input modifiers in rules such as `Control+P -> up_arrow`
      // and `Control+N -> down_arrow`. The rule temporarily clears Control before sending the
      // arrow and restores it lazily in the queue. When Control is held and P then N are pressed,
      // the N rule clears Control again before the next HID sync, so both arrows are sent without
      // Control and there is no extra Control down/up between them. If the next key uses Control
      // normally, the restored Control state is sent before that key_down.
      bool lazy;

      // With true, the modifier affects HID output and matches downstream rules only for
      // events from the same source. With false, it participates in ordinary modifier matching.
      // For `1 -> Shift+A`, the generated A can match a downstream Shift+A rule, while a
      // separate physical B does not match Shift+B because of the generated Shift.
      // Use the same value as the corresponding key_down so the release clears the correct state.
      bool output_only_modifier;

      // Controls when the release is posted to the event queue. With true, a later unrelated
      // input or output can trigger an early, non-lazy release. For `1 -> Shift+A`, this lets
      // Shift stay down for `AAAA...` but sends Shift up before B if B is pressed while 1 is held.
      // With false, the release waits for the source key_up. For example, when macOS's
      // "Use F1, F2, etc. keys as standard function keys" setting is off, a fn_function_keys
      // mapping such as `f3 -> f3` adds fn automatically if the output does not specify it.
      // Keeping fn down means macOS also sees it on B, even though the user only pressed F3+B.
      // Releasing fn before B would make macOS report F3 as released while it is still physically
      // held, breaking F3+B. Keep fn down until F3 is released. Set output_only_modifier to true
      // so B still does not match a Karabiner rule that requires fn+B.
      bool release_when_other_key_is_pressed;
    };

    explicit entry(const parameters& parameters)
        : device_id_(parameters.device_id),
          event_(parameters.event),
          event_type_(parameters.event_type),
          original_event_(parameters.original_event),
          lazy_(parameters.lazy),
          output_only_modifier_(parameters.output_only_modifier),
          release_when_other_key_is_pressed_(parameters.release_when_other_key_is_pressed) {
    }

    event_queue::entry make_entry(const event_queue::event_time_stamp& event_time_stamp,
                                  bool force_non_lazy = false) const {
      return event_queue::entry({
          .device_id = device_id_,
          .event_time_stamp = event_time_stamp,
          .event = event_,
          .event_type = event_type_,
          .original_event = original_event_,
          .state = event_queue::state::manipulated,
          .lazy = force_non_lazy ? false : lazy_,
          .output_only_modifier = output_only_modifier_,
          // release_when_other_key_is_pressed_ is only used to select pending releases for
          // early posting. The caller passes force_non_lazy for those releases so the virtual
          // device applies the modifier change before the next output.
      });
    }

    [[nodiscard]] bool get_release_when_other_key_is_pressed() const {
      return release_when_other_key_is_pressed_;
    }

  private:
    device_id device_id_;
    event_queue::event event_;
    event_type event_type_;
    event_queue::event original_event_;
    bool lazy_;
    bool output_only_modifier_;
    bool release_when_other_key_is_pressed_;
  };

  [[nodiscard]] const std::vector<entry>& get_events() const {
    return events_;
  }

  void emplace_back_event(const entry::parameters& parameters) {
    events_.emplace_back(parameters);
  }

  std::vector<entry> extract_events_when_other_key_is_pressed() {
    std::vector<entry> result;

    auto it = std::begin(events_);
    while (it != std::end(events_)) {
      if (it->get_release_when_other_key_is_pressed()) {
        result.push_back(*it);
        it = events_.erase(it);
      } else {
        ++it;
      }
    }

    return result;
  }

  void clear_events() {
    events_.clear();
  }

private:
  std::vector<entry> events_;
};
} // namespace krbn::manipulator::manipulators::basic::manipulated_original_event
