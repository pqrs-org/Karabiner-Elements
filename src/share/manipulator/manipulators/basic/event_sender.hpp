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

inline to_event_definitions filter_by_conditions(const to_event_definitions& to_events,
                                                 const event_queue::entry& entry,
                                                 const manipulator_environment& manipulator_environment) {
  to_event_definitions result;
  std::copy_if(to_events.begin(),
               to_events.end(),
               std::back_inserter(result),
               [&entry, &manipulator_environment](auto&& e) {
                 return e->get_condition_manager().is_fulfilled(entry, manipulator_environment);
               });
  return result;
}

inline bool is_last_to_event_modifier_key_event(const to_event_definitions& to_events) {
  if (to_events.empty()) {
    return false;
  }

  if (auto e = to_events.back()->get_event_definition().get_if<momentary_switch_event>()) {
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
                                    const to_event_definitions& to_events,
                                    manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                    absolute_time_duration& time_stamp_delay,
                                    event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return;
  }

  auto filtered_to_events = filter_by_conditions(to_events,
                                                 front_input_event,
                                                 output_event_queue.get_manipulator_environment());

  for (auto it = std::begin(filtered_to_events); it != std::end(filtered_to_events); std::advance(it, 1)) {
    auto to = *it;
    if (auto event = to->get_event_definition().to_event()) {
      // to_modifier down, to_key down, to_key up, to_modifier up

      auto to_modifier_events = to->make_modifier_events();

      bool is_modifier_key_event = false;
      if (auto e = event->get_if<momentary_switch_event>()) {
        if (e->modifier_flag()) {
          is_modifier_key_event = true;
        } else if (e->caps_lock()) {
          is_modifier_key_event = true;
        }
      }
      if (auto e = event->get_if<mouse_key>()) {
        if (e->is_speed_multiplier()) {
          is_modifier_key_event = true;
        }
      }

      {
        // Unset lazy if event is modifier key event in order to keep modifier keys order.

        bool lazy = !is_modifier_key_event || to->get_lazy();
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
                                              to->get_lazy());

        if (to->get_halt()) {
          current_manipulated_original_event.set_halted();
        }
      }

      // Post key_up event

      if (it != std::end(filtered_to_events) - 1 || !to->get_repeat()) {
        time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(to->get_hold_down_milliseconds());

        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              to->get_lazy());
      } else {
        current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                     *event,
                                                                                     event_type::key_up,
                                                                                     front_input_event.get_original_event(),
                                                                                     to->get_lazy());
      }

      {
        for (const auto& e : to_modifier_events) {
          if (it == std::end(filtered_to_events) - 1 && is_modifier_key_event) {
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
                                 const to_event_definitions& to_events,
                                 manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                 absolute_time_duration& time_stamp_delay,
                                 event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return;
  }

  auto filtered_to_events = filter_by_conditions(to_events,
                                                 front_input_event,
                                                 output_event_queue.get_manipulator_environment());

  for (auto it = std::begin(filtered_to_events); it != std::end(filtered_to_events); std::advance(it, 1)) {
    auto to = *it;
    if (auto event = to->get_event_definition().to_event()) {
      auto to_modifier_events = to->make_modifier_events();

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
                                              to->get_lazy());

        if (to->get_halt()) {
          current_manipulated_original_event.set_halted();
        }
      }

      // Post key_up event

      {
        time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(to->get_hold_down_milliseconds());

        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry(front_input_event.get_device_id(),
                                              t,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              event_queue::state::manipulated,
                                              to->get_lazy());
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

inline void post_active_modifier_flags(const event_queue::entry& front_input_event,
                                       const std::vector<modifier_flag_manager::active_modifier_flag>& active_modifier_flags,
                                       absolute_time_duration& time_stamp_delay,
                                       event_queue::queue& output_event_queue) {
  for (const auto& f : active_modifier_flags) {
    base::post_lazy_modifier_key_event(momentary_switch_event(f.get_modifier_flag()),
                                       f.get_count() > 0 ? event_type::key_down : event_type::key_up,
                                       f.get_device_id(),
                                       front_input_event.get_event_time_stamp(),
                                       time_stamp_delay,
                                       front_input_event.get_original_event(),
                                       output_event_queue);
  }
}

class scoped_from_key_modifier_flags_state_restorer final {
public:
  scoped_from_key_modifier_flags_state_restorer(const event_queue::entry& front_input_event,
                                                manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                absolute_time_duration& time_stamp_delay,
                                                event_queue::queue& output_event_queue)
      : front_input_event_(front_input_event),
        current_manipulated_original_event_(current_manipulated_original_event),
        time_stamp_delay_(time_stamp_delay),
        output_event_queue_(output_event_queue) {
    std::vector<modifier_flag_manager::active_modifier_flag> scoped_active_modifier_flags;

    {
      // It's not only necessary to change the state of the output_event_queue::modifier_flag_manager, but also to send the key event here.
      // So, once we use scoped_modifier_flags to find out which key events we need to send, and then send them.
      modifier_flag_manager::scoped_modifier_flags scoped_modifier_flags(output_event_queue_.get_modifier_flag_manager(),
                                                                         current_manipulated_original_event_.get_key_down_modifier_flags());
      scoped_active_modifier_flags = scoped_modifier_flags.get_scoped_active_modifier_flags();
      inverse_active_modifier_flags_ = scoped_modifier_flags.get_inverse_active_modifier_flags();
    }

    event_sender::post_active_modifier_flags(front_input_event_,
                                             scoped_active_modifier_flags,
                                             time_stamp_delay_,
                                             output_event_queue_);
  }

  ~scoped_from_key_modifier_flags_state_restorer(void) {
    //
    // Revert scoped modifier flags changes.
    //

    event_sender::post_active_modifier_flags(front_input_event_,
                                             inverse_active_modifier_flags_,
                                             time_stamp_delay_,
                                             output_event_queue_);
  }

private:
  const event_queue::entry& front_input_event_;
  manipulated_original_event::manipulated_original_event& current_manipulated_original_event_;
  absolute_time_duration& time_stamp_delay_;
  event_queue::queue& output_event_queue_;

  std::vector<modifier_flag_manager::active_modifier_flag> inverse_active_modifier_flags_;
};

} // namespace event_sender
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
