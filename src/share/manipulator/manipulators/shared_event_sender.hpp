#pragma once

#include "base.hpp"

namespace krbn::manipulator::manipulators::shared_event_sender {
inline void post_active_modifier_flags(const std::vector<modifier_flag_manager::active_modifier_flag>& active_modifier_flags,
                                       const event_queue::event_time_stamp& event_time_stamp,
                                       absolute_time_duration& time_stamp_delay,
                                       const event_queue::event& original_event,
                                       event_queue::queue& output_event_queue) {
  for (const auto& f : active_modifier_flags) {
    base::post_lazy_modifier_key_event({
        .momentary_switch_event = momentary_switch_event(f.get_modifier_flag()),
        .event_type = f.get_count() > 0 ? event_type::key_down : event_type::key_up,
        .device_id = f.get_device_id(),
        .event_time_stamp = event_time_stamp,
        .time_stamp_delay = time_stamp_delay,
        .original_event = original_event,
        .output_event_queue = output_event_queue,
    });
  }
}

// Emit one complete tap, including its temporary output modifiers.
inline void post_tap(const to_event_definition& to,
                     device_id device_id,
                     const event_queue::event_time_stamp& event_time_stamp,
                     absolute_time_duration& time_stamp_delay,
                     const event_queue::event& original_event,
                     event_queue::queue& output_event_queue) {
  if (auto event = to.get_event_definition().to_event()) {
    auto to_modifier_events = to.make_modifier_events();

    //
    // Post modifier events
    //

    for (const auto& e : to_modifier_events) {
      base::post_lazy_modifier_key_event({
          .momentary_switch_event = e,
          .event_type = event_type::key_down,
          .device_id = device_id,
          .event_time_stamp = event_time_stamp,
          .time_stamp_delay = time_stamp_delay,
          .original_event = original_event,
          .output_event_queue = output_event_queue,
      });
    }

    //
    // Post key_down event
    //

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
          .lazy = to.get_lazy(),
      });
    }

    //
    // Post key_up event
    //

    {
      time_stamp_delay += pqrs::osx::chrono::make_absolute_time_duration(to.get_hold_down_milliseconds());

      auto t = event_time_stamp;
      t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

      output_event_queue.emplace_back_entry({
          .device_id = device_id,
          .event_time_stamp = t,
          .event = *event,
          .event_type = event_type::key_up,
          .original_event = original_event,
          .state = event_queue::state::manipulated,
          .lazy = to.get_lazy(),
      });
    }

    //
    // Post modifier events
    //

    for (const auto& e : to_modifier_events) {
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
} // namespace krbn::manipulator::manipulators::shared_event_sender
