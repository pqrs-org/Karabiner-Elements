#pragma once

#include "../../types.hpp"
#include "../shared_event_sender.hpp"
#include "event_queue.hpp"
#include "manipulated_original_event/manipulated_original_event.hpp"
#include "types.hpp"

namespace krbn::manipulator::manipulators::basic::event_sender {

// Describes whether `post_events_at_key_down` produced an effective output
// that should interrupt a pending `to_if_alone`.
// `not_required` includes empty or filtered output and invalid events such as `vk_none`.
// `required` includes valid events and output modifiers.
enum class to_if_alone_cancellation {
  not_required,
  required,
};

inline to_event_definitions filter_and_replace_events(const to_event_definitions& to_events,
                                                      const conditions::condition_context& condition_context,
                                                      manipulator_environment& manipulator_environment,
                                                      manipulated_original_event::manipulated_original_event& current_manipulated_original_event) {
  to_event_definitions result;

  for (const auto& to : to_events) {
    //
    // Filter
    //

    if (!to->get_condition_manager().is_fulfilled(condition_context,
                                                  manipulator_environment)) {
      continue;
    }

    //
    // Replace
    //

    // If no replacement is needed, copy it as-is.
    if (to->get_event_definition().get_type() != event_definition::type::from_event) {
      result.push_back(to);
      continue;
    }

    for (const auto& from_event : current_manipulated_original_event.get_from_events()) {
      if (auto e = from_event.get_event().get_if<momentary_switch_event>()) {
        result.push_back(to_event_definition::make_from_momentary_switch_event(*e, *to));
      }
    }
  }

  return result;
}

inline bool to_events_release_deferred_modifiers(
    const to_event_definitions& to_events,
    const conditions::condition_context& condition_context,
    manipulator_environment& manipulator_environment,
    manipulated_original_event::manipulated_original_event& current_manipulated_original_event) {
  for (const auto& to : filter_and_replace_events(to_events,
                                                  condition_context,
                                                  manipulator_environment,
                                                  current_manipulated_original_event)) {
    if (!to->make_modifier_events().empty()) {
      return true;
    }

    if (auto event = to->get_event_definition().to_event()) {
      if (manipulators::deferred_output_session::releases_deferred_modifiers(*event)) {
        return true;
      }
    }
  }

  return false;
}

// Run an output operation through `deferred_output_session` when the filtered
// definitions contain an effective output. Invalid events such as `vk_none`
// execute the operation without releasing a modifier held for key repeat.
template <typename F>
decltype(auto) post_output(
    const to_event_definitions& to_events,
    const conditions::condition_context& condition_context,
    manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
    const std::shared_ptr<const deferred_output_session>& deferred_output_session,
    const event_queue::entry& front_input_event,
    event_queue::queue& output_event_queue,
    F&& post_events) {
  if (deferred_output_session &&
      to_events_release_deferred_modifiers(
          to_events,
          condition_context,
          output_event_queue.get_manipulator_environment(),
          current_manipulated_original_event)) {
    return deferred_output_session->post(front_input_event,
                                         output_event_queue,
                                         std::forward<F>(post_events));
  }

  return std::forward<F>(post_events)(output_event_queue);
}

[[nodiscard]] inline bool is_last_to_event_modifier_key_event(const to_event_definitions& to_events) {
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

inline void post_from_mandatory_modifiers_key_up(manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                 device_id device_id,
                                                 const event_queue::event_time_stamp& event_time_stamp,
                                                 absolute_time_duration& time_stamp_delay,
                                                 const event_queue::event& original_event,
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

  base::post_lazy_modifier_key_events({
      .modifiers = modifiers,
      .event_type = event_type::key_up,
      .device_id = device_id,
      .event_time_stamp = event_time_stamp,
      .time_stamp_delay = time_stamp_delay,
      .original_event = original_event,
      .output_event_queue = output_event_queue,
  });
}

inline void post_from_mandatory_modifiers_key_down(manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                   device_id device_id,
                                                   const event_queue::event_time_stamp& event_time_stamp,
                                                   absolute_time_duration& time_stamp_delay,
                                                   const event_queue::event& original_event,
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

  base::post_lazy_modifier_key_events({
      .modifiers = modifiers,
      .event_type = event_type::key_down,
      .device_id = device_id,
      .event_time_stamp = event_time_stamp,
      .time_stamp_delay = time_stamp_delay,
      .original_event = original_event,
      .output_event_queue = output_event_queue,
  });
}

[[nodiscard]] inline to_if_alone_cancellation post_events_at_key_down(const to_event_definitions& to_events,
                                                                      manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                                      const conditions::condition_context& condition_context,
                                                                      device_id device_id,
                                                                      const event_queue::event_time_stamp& event_time_stamp,
                                                                      absolute_time_duration& time_stamp_delay,
                                                                      const event_queue::event& original_event,
                                                                      event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return to_if_alone_cancellation::not_required;
  }

  auto result = to_if_alone_cancellation::not_required;

  auto filtered_to_events = filter_and_replace_events(to_events,
                                                      condition_context,
                                                      output_event_queue.get_manipulator_environment(),
                                                      current_manipulated_original_event);

  for (auto it = std::begin(filtered_to_events); it != std::end(filtered_to_events); std::advance(it, 1)) {
    auto to = *it;
    if (auto event = to->get_event_definition().to_event()) {
      // to_modifier down, to_key down, to_key up, to_modifier up

      auto to_modifier_events = to->make_modifier_events();

      if (!to_modifier_events.empty()) {
        result = to_if_alone_cancellation::required;
      } else if (auto e = event->get_if<momentary_switch_event>()) {
        // Invalid events such as `vk_none` do not require cancelling `to_if_alone`.
        if (e->valid()) {
          result = to_if_alone_cancellation::required;
        }
      } else {
        result = to_if_alone_cancellation::required;
      }

      bool is_modifier_key_event = false;
      bool is_repeatable_key_event = false;
      if (auto e = event->get_if<momentary_switch_event>()) {
        if (e->modifier_flag()) {
          is_modifier_key_event = true;
        } else if (e->caps_lock()) {
          is_modifier_key_event = true;
        } else {
          is_repeatable_key_event = e->interrupts_key_repeat();
        }
      }
      if (auto e = event->get_if<mouse_key>()) {
        if (e->is_speed_multiplier()) {
          is_modifier_key_event = true;
        }
      }

      std::unordered_set<modifier_flag> additional_modifiers_for_matching;

      {
        // Unset lazy if event is modifier key event in order to keep modifier keys order.

        bool lazy = !is_modifier_key_event || to->get_lazy();
        bool output_only_modifier = (it == std::end(filtered_to_events) - 1 &&
                                     is_repeatable_key_event &&
                                     to->get_repeat());
        for (const auto& e : to_modifier_events) {
          // An output-only modifier is excluded from the queue's modifier
          // state for matching so that it does not affect an unrelated next
          // input event. However, it must still participate in downstream
          // matching for the output key generated together with it.
          //
          // For example, with `a -> shift+1` followed by `shift+1 -> b`, the
          // generated `1` must be matched with shift, while a subsequently
          // pressed physical key must not inherit that shift.
          if (output_only_modifier) {
            if (auto modifier = e.make_modifier_flag()) {
              additional_modifiers_for_matching.insert(*modifier);
            }
          }

          base::post_lazy_modifier_key_event({
              .momentary_switch_event = e,
              .event_type = event_type::key_down,
              .device_id = device_id,
              .event_time_stamp = event_time_stamp,
              .time_stamp_delay = time_stamp_delay,
              .original_event = original_event,
              .output_event_queue = output_event_queue,
              .lazy = lazy,
              .output_only_modifier = output_only_modifier,
          });
        }
      }

      // Post key_down event

      {
        auto t = event_time_stamp;
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry({
            .device_id = device_id,
            .event_time_stamp = t,
            .event = *event,
            .event_type = event_type::key_down,
            .original_event = original_event,
            .state = event_queue::state::manipulated,
            .lazy = to->get_lazy(),
            .additional_modifiers_for_matching = additional_modifiers_for_matching,
        });

        if (to->get_halt()) {
          current_manipulated_original_event.set_halted();
        }
      }

      // Post key_up event

      if (it != std::end(filtered_to_events) - 1 || !to->get_repeat()) {
        time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(to->get_hold_down_milliseconds());

        auto t = event_time_stamp;
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue.emplace_back_entry({
            .device_id = device_id,
            .event_time_stamp = t,
            .event = *event,
            .event_type = event_type::key_up,
            .original_event = original_event,
            .state = event_queue::state::manipulated,
            .lazy = to->get_lazy(),
        });
      } else {
        current_manipulated_original_event.get_events_at_key_up().emplace_back_event(device_id,
                                                                                     *event,
                                                                                     event_type::key_up,
                                                                                     original_event,
                                                                                     to->get_lazy());
      }

      {
        for (const auto& e : to_modifier_events) {
          if (it == std::end(filtered_to_events) - 1 &&
              (is_modifier_key_event ||
               (is_repeatable_key_event && to->get_repeat()))) {
            auto pair = base::make_lazy_modifier_key_event(e, event_type::key_up);

            current_manipulated_original_event.get_events_at_key_up().emplace_back_event(device_id,
                                                                                         pair.first,
                                                                                         pair.second,
                                                                                         original_event,
                                                                                         true,
                                                                                         is_repeatable_key_event && to->get_repeat());
          } else {
            base::post_lazy_modifier_key_event({
                .momentary_switch_event = e,
                .event_type = event_type::key_up,
                .device_id = device_id,
                .event_time_stamp = event_time_stamp,
                .time_stamp_delay = time_stamp_delay,
                .original_event = original_event,
                .output_event_queue = output_event_queue,
            });
          }
        }
      }
    }
  }

  return result;
}

inline void post_events_at_key_up(manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                  const event_queue::event_time_stamp& event_time_stamp,
                                  absolute_time_duration& time_stamp_delay,
                                  event_queue::queue& output_event_queue) {
  for (const auto& e : current_manipulated_original_event.get_events_at_key_up().get_events()) {
    auto t = event_time_stamp;
    t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);
    output_event_queue.push_back_entry(e.make_entry(t));
  }
  current_manipulated_original_event.get_events_at_key_up().clear_events();
}

inline void post_events_at_key_up_when_other_key_is_pressed(
    manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
    const event_queue::event_time_stamp& event_time_stamp,
    absolute_time_duration& time_stamp_delay,
    event_queue::queue& output_event_queue) {
  for (const auto& e : current_manipulated_original_event.get_events_at_key_up().extract_events_when_other_key_is_pressed()) {
    auto t = event_time_stamp;
    t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);
    output_event_queue.push_back_entry(e.make_entry(t, true));
  }
}

inline void post_extra_to_events(const to_event_definitions& to_events,
                                 manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                 const conditions::condition_context& condition_context,
                                 device_id device_id,
                                 const event_queue::event_time_stamp& event_time_stamp,
                                 absolute_time_duration& time_stamp_delay,
                                 const event_queue::event& original_event,
                                 event_queue::queue& output_event_queue) {
  if (current_manipulated_original_event.get_halted()) {
    return;
  }

  auto filtered_to_events = filter_and_replace_events(to_events,
                                                      condition_context,
                                                      output_event_queue.get_manipulator_environment(),
                                                      current_manipulated_original_event);

  for (auto it = std::begin(filtered_to_events); it != std::end(filtered_to_events); std::advance(it, 1)) {
    auto to = *it;
    manipulators::shared_event_sender::post_tap(*to,
                                                device_id,
                                                event_time_stamp,
                                                time_stamp_delay,
                                                original_event,
                                                output_event_queue);

    if (to->get_halt() && to->get_event_definition().to_event()) {
      current_manipulated_original_event.set_halted();
    }
  }
}

class scoped_from_key_modifier_flags_state_restorer final {
public:
  scoped_from_key_modifier_flags_state_restorer(manipulated_original_event::manipulated_original_event& current_manipulated_original_event,
                                                const event_queue::event_time_stamp& event_time_stamp,
                                                absolute_time_duration& time_stamp_delay,
                                                const event_queue::event& original_event,
                                                event_queue::queue& output_event_queue)
      : current_manipulated_original_event_(current_manipulated_original_event),
        event_time_stamp_(event_time_stamp),
        time_stamp_delay_(time_stamp_delay),
        original_event_(original_event),
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

    shared_event_sender::post_active_modifier_flags(scoped_active_modifier_flags,
                                                    event_time_stamp_,
                                                    time_stamp_delay_,
                                                    original_event_,
                                                    output_event_queue_);
  }

  ~scoped_from_key_modifier_flags_state_restorer() {
    //
    // Revert scoped modifier flags changes.
    //

    shared_event_sender::post_active_modifier_flags(inverse_active_modifier_flags_,
                                                    event_time_stamp_,
                                                    time_stamp_delay_,
                                                    original_event_,
                                                    output_event_queue_);
  }

private:
  manipulated_original_event::manipulated_original_event& current_manipulated_original_event_;
  const event_queue::event_time_stamp& event_time_stamp_;
  absolute_time_duration& time_stamp_delay_;
  const event_queue::event& original_event_;
  event_queue::queue& output_event_queue_;

  std::vector<modifier_flag_manager::active_modifier_flag> inverse_active_modifier_flags_;
};

} // namespace krbn::manipulator::manipulators::basic::event_sender
