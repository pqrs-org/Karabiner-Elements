#pragma once

#include "../../types.hpp"
#include "event_queue.hpp"
#include "manipulated_original_event/manipulated_original_event.hpp"
#include "types.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
namespace event_sender {
inline bool is_last_to_event_modifier_key_event(const std::vector<to_event_definition>& to_events) {
  if (to_events.empty()) {
    return false;
  }

  if (auto e = to_events.back().get_event_definition().get_if<momentary_switch_event>()) {
    if (e->modifier_flag()) {
      return true;
    }
  }

  return false;
}

inline void post_from_mandatory_modifiers_key_up(const event_queue::entry& front_input_event,
                                                 manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                 absolute_time_duration& time_stamp_delay,
                                                 event_queue::queue& output_event_queue) {
  // ----------------------------------------
  // Make target modifiers

  std::unordered_set<modifier_flag> modifiers;

  for (const auto& m : current_manipulated_original_event.get_from_mandatory_modifiers()) {
    auto& key_up_posted_from_mandatory_modifiers = current_manipulated_original_event.get_key_up_posted_from_mandatory_modifiers();

    if (key_up_posted_from_mandatory_modifiers.find(m) != std::end(key_up_posted_from_mandatory_modifiers)) {
      continue;
    }

    // All from_mandatory_modifiers are usually pressed when `post_from_mandatory_modifiers_key_up` is called.
    // However, there are some exceptional cases.
    //
    // Example:
    //   - from:            left_shift+f2
    //   - to:              left_shift+f3
    //   - to_after_key_up: left_shift+tab
    //
    //   1. left_shift key_down
    //   2. f2 key_down
    //   3. left_shift key_up
    //   4. f2 key_up
    //
    //   to_after_key_up is called at 4.
    //   to_after_key_up calls `post_from_mandatory_modifiers_key_up` but `left_shift` is not pressed.
    //
    // We should not post key_up event in this case.

    if (!output_event_queue.get_modifier_flag_manager().is_pressed(m)) {
      continue;
    }

    modifiers.insert(m);
    key_up_posted_from_mandatory_modifiers.insert(m);
  }

  // ----------------------------------------

  base::post_lazy_modifier_key_events(modifiers,
                                      event_type::key_up,
                                      front_input_event.get_device_id(),
                                      front_input_event.get_event_time_stamp(),
                                      time_stamp_delay,
                                      front_input_event.get_original_event(),
                                      output_event_queue);
}

inline void post_from_mandatory_modifiers_key_down(const event_queue::entry& front_input_event,
                                                   manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                   absolute_time_duration& time_stamp_delay,
                                                   event_queue::queue& output_event_queue) {
  // ----------------------------------------
  // Make target modifiers

  std::unordered_set<modifier_flag> modifiers;

  for (const auto& m : current_manipulated_original_event.get_from_mandatory_modifiers()) {
    auto& key_up_posted_from_mandatory_modifiers = current_manipulated_original_event.get_key_up_posted_from_mandatory_modifiers();

    if (key_up_posted_from_mandatory_modifiers.find(m) == std::end(key_up_posted_from_mandatory_modifiers)) {
      continue;
    }

    modifiers.insert(m);
    key_up_posted_from_mandatory_modifiers.erase(m);
  }

  // ----------------------------------------

  base::post_lazy_modifier_key_events(modifiers,
                                      event_type::key_down,
                                      front_input_event.get_device_id(),
                                      front_input_event.get_event_time_stamp(),
                                      time_stamp_delay,
                                      front_input_event.get_original_event(),
                                      output_event_queue);
}

inline void post_events_at_key_down(const event_queue::entry& front_input_event,
                                    std::vector<to_event_definition> to_events,
                                    manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                    absolute_time_duration& time_stamp_delay,
                                    event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return;
  }

  for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
    if (auto event = it->get_event_definition().to_event()) {
      // to_modifier down, to_key down, to_key up, to_modifier up

      auto to_modifier_events = it->make_modifier_events();

      bool is_modifier_key_event = false;
      if (auto e = event->get_if<momentary_switch_event>()) {
        if (e->modifier_flag()) {
          is_modifier_key_event = true;
        } else if (e->caps_lock()) {
          is_modifier_key_event = true;
        }
      }

      {
        // Unset lazy if event is modifier key event in order to keep modifier keys order.

        bool lazy = !is_modifier_key_event || it->get_lazy();
        for (const auto& e : to_modifier_events) {
          base::post_lazy_modifier_key_event(e,
                                             event_type::key_down,
                                             front_input_event.get_device_id(),
                                             front_input_event.get_event_time_stamp(),
                                             time_stamp_delay,
                                             front_input_event.get_original_event(),
                                             output_event_queue,
                                             lazy);
        }
      }

      // Post key_down event

      {
        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_down,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              it->get_lazy());

        if (it->get_halt()) {
          current_manipulated_original_event.set_halted();
        }
      }

      // Post key_up event

      if (it != std::end(to_events) - 1 || !it->get_repeat()) {
        time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(it->get_hold_down_milliseconds());

        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              it->get_lazy());
      } else {
        current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                     *event,
                                                                                     event_type::key_up,
                                                                                     front_input_event.get_original_event(),
                                                                                     it->get_lazy());
      }

      {
        for (const auto& e : to_modifier_events) {
          if (it == std::end(to_events) - 1 && is_modifier_key_event) {
            auto pair = base::make_lazy_modifier_key_event(e, event_type::key_up);

            current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                         pair.first,
                                                                                         pair.second,
                                                                                         front_input_event.get_original_event(),
                                                                                         true);
          } else {
            base::post_lazy_modifier_key_event(e,
                                               event_type::key_up,
                                               front_input_event.get_device_id(),
                                               front_input_event.get_event_time_stamp(),
                                               time_stamp_delay,
                                               front_input_event.get_original_event(),
                                               output_event_queue);
          }
        }
      }
    }
  }
}

inline void post_events_at_key_up(const event_queue::entry& front_input_event,
                                  manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                  absolute_time_duration& time_stamp_delay,
                                  event_queue::queue& output_event_queue) {
  for (const auto& e : current_manipulated_original_event.get_events_at_key_up().get_events()) {
    auto t = front_input_event.get_event_time_stamp();
    t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);
    output_event_queue.push_back_entry(e.make_entry(t));
  }
  current_manipulated_original_event.get_events_at_key_up().clear_events();
}

inline void post_extra_to_events(const event_queue::entry& front_input_event,
                                 const std::vector<to_event_definition>& to_events,
                                 manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                 absolute_time_duration& time_stamp_delay,
                                 event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return;
  }

  for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
    auto& to = *it;
    if (auto event = to.get_event_definition().to_event()) {
      auto to_modifier_events = to.make_modifier_events();

      // Post modifier events

      for (const auto& e : to_modifier_events) {
        base::post_lazy_modifier_key_event(e,
                                           event_type::key_down,
                                           front_input_event.get_device_id(),
                                           front_input_event.get_event_time_stamp(),
                                           time_stamp_delay,
                                           front_input_event.get_original_event(),
                                           output_event_queue);
      }

      // Post key_down event

      {
        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_down,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              it->get_lazy());

        if (it->get_halt()) {
          current_manipulated_original_event.set_halted();
        }
      }

      // Post key_up event

      {
        time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(it->get_hold_down_milliseconds());

        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              it->get_lazy());
      }

      // Post modifier events

      for (const auto& e : to_modifier_events) {
        base::post_lazy_modifier_key_event(e,
                                           event_type::key_up,
                                           front_input_event.get_device_id(),
                                           front_input_event.get_event_time_stamp(),
                                           time_stamp_delay,
                                           front_input_event.get_original_event(),
                                           output_event_queue);
      }
    }
  }
}
} // namespace event_sender
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
