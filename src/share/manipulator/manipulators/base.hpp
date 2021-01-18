#pragma once

#include "../condition_manager.hpp"
#include "../types.hpp"
#include "event_queue.hpp"
#include "modifier_flag_manager.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
class base {
protected:
  base(void) : validity_(validity::valid) {
  }

public:
  virtual ~base(void) {
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) = 0;

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       std::shared_ptr<event_queue::queue> output_event_queue,
                                       absolute_time_point now) = 0;

  virtual bool active(void) const = 0;

  virtual bool needs_virtual_hid_pointing(void) const = 0;

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) = 0;

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) = 0;

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) = 0;

  validity get_validity(void) const {
    return validity_;
  }
  virtual void set_validity(validity value) {
    validity_ = value;
  }

  void push_back_condition(std::shared_ptr<manipulator::conditions::base> condition) {
    condition_manager_.push_back_condition(condition);
  }

  static void post_lazy_modifier_key_events(const std::unordered_set<modifier_flag>& modifiers,
                                            event_type event_type,
                                            device_id device_id,
                                            const event_queue::event_time_stamp& event_time_stamp,
                                            absolute_time_duration& time_stamp_delay,
                                            const event_queue::event& original_event,
                                            event_queue::queue& output_event_queue) {
    for (const auto& m : modifiers) {
      post_lazy_modifier_key_event(momentary_switch_event(m),
                                   event_type,
                                   device_id,
                                   event_time_stamp,
                                   time_stamp_delay,
                                   original_event,
                                   output_event_queue);
    }
  }

  static void post_lazy_modifier_key_event(const momentary_switch_event& momentary_switch_event,
                                           event_type event_type,
                                           device_id device_id,
                                           const event_queue::event_time_stamp& event_time_stamp,
                                           absolute_time_duration& time_stamp_delay,
                                           const event_queue::event& original_event,
                                           event_queue::queue& output_event_queue,
                                           bool lazy = true) {
    if (momentary_switch_event.valid()) {
      auto t = event_time_stamp;
      t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

      auto pair = make_lazy_modifier_key_event(momentary_switch_event, event_type);

      event_queue::entry entry(device_id,
                               t,
                               pair.first,
                               pair.second,
                               original_event,
                               event_queue::state::manipulated,
                               lazy);
      output_event_queue.push_back_entry(entry);
    }
  }

  static std::pair<event_queue::event, event_type> make_lazy_modifier_key_event(const momentary_switch_event& momentary_switch_event,
                                                                                event_type event_type) {
    auto event = event_queue::event(momentary_switch_event);
    if (momentary_switch_event.caps_lock()) {
      // Send sticky_modifier event for caps_lock.
      event = event_queue::event::make_sticky_modifier_event(modifier_flag::caps_lock,
                                                             event_type == event_type::key_down
                                                                 ? sticky_modifier_type::on
                                                                 : sticky_modifier_type::off);
      event_type = event_type::single;
    }

    return std::make_pair(event, event_type);
  }

protected:
  validity validity_;
  condition_manager condition_manager_;
};
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
