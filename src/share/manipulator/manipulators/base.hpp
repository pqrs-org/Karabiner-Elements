#pragma once

#include "../condition_manager.hpp"
#include "../types.hpp"
#include "deferred_output_session.hpp"
#include "event_queue.hpp"
#include "modifier_flag_manager.hpp"

namespace krbn::manipulator::manipulators {
class base {
protected:
  base() = default;

public:
  virtual ~base() {
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) = 0;

  virtual void cancel_to_if_alone(const event_queue::entry& front_input_event) {
  }

  virtual void post_deferred_modifier_key_up_events(const event_queue::entry& front_input_event,
                                                    event_queue::queue& output_event_queue,
                                                    deferred_modifier_key_up_event_scope scope) {
  }

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       std::shared_ptr<event_queue::queue> output_event_queue,
                                       absolute_time_point now,
                                       std::shared_ptr<const deferred_output_session> deferred_output_session = nullptr) = 0;

  virtual bool active() const = 0;

  virtual bool needs_virtual_hid_pointing() const = 0;

  [[nodiscard]] virtual bool uses_caps_lock_led_override() const = 0;

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) = 0;

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) = 0;

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) = 0;

  [[nodiscard]] validity get_validity() const {
    return validity_;
  }
  // manipulator_manager::invalidate_manipulators calls this with validity::invalid
  // when replacing existing manipulators, for example after a configuration reload.
  // Normal event processing in manipulate does not call this setter, even when
  // input conditions stop matching or modifier keys are released. The manipulator
  // remains available for later matching input.
  // This is separate from event_queue::entry::set_validity, which manipulate may
  // call to mark an input event as consumed.
  virtual void set_validity(validity value) {
    validity_ = value;
  }

  void push_back_condition(std::shared_ptr<manipulator::conditions::base> condition) {
    condition_manager_.push_back_condition(condition);
  }

  struct post_lazy_modifier_key_events_parameters final {
    const std::unordered_set<modifier_flag>& modifiers;
    krbn::event_type event_type;
    krbn::device_id device_id;
    const event_queue::event_time_stamp& event_time_stamp;
    absolute_time_duration& time_stamp_delay;
    const event_queue::event& original_event;
    event_queue::queue& output_event_queue;
  };

  struct post_lazy_modifier_key_event_parameters final {
    const krbn::momentary_switch_event& momentary_switch_event;
    krbn::event_type event_type;
    krbn::device_id device_id;
    const event_queue::event_time_stamp& event_time_stamp;
    absolute_time_duration& time_stamp_delay;
    const event_queue::event& original_event;
    event_queue::queue& output_event_queue;
    bool lazy{true};
    bool output_only_modifier{false};
  };

  static void post_lazy_modifier_key_events(const post_lazy_modifier_key_events_parameters& parameters) {
    for (const auto& m : parameters.modifiers) {
      post_lazy_modifier_key_event({
          .momentary_switch_event = momentary_switch_event(m),
          .event_type = parameters.event_type,
          .device_id = parameters.device_id,
          .event_time_stamp = parameters.event_time_stamp,
          .time_stamp_delay = parameters.time_stamp_delay,
          .original_event = parameters.original_event,
          .output_event_queue = parameters.output_event_queue,
      });
    }
  }

  static void post_lazy_modifier_key_event(const post_lazy_modifier_key_event_parameters& parameters) {
    if (parameters.momentary_switch_event.valid()) {
      auto t = parameters.event_time_stamp;
      t.set_time_stamp(t.get_time_stamp() + parameters.time_stamp_delay++);

      auto pair = make_lazy_modifier_key_event(parameters.momentary_switch_event,
                                               parameters.event_type);

      event_queue::entry entry({
          .device_id = parameters.device_id,
          .event_time_stamp = t,
          .event = pair.first,
          .event_type = pair.second,
          .original_event = parameters.original_event,
          .state = event_queue::state::manipulated,
          .lazy = parameters.lazy,
          .output_only_modifier = parameters.output_only_modifier,
      });
      parameters.output_event_queue.push_back_entry(entry);
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
  // Invalidation stops new manipulations without immediately destroying state
  // needed to finish existing ones. For example, if configuration is reloaded
  // while a key is held, basic rejects new key_down events but retains the state
  // needed to handle the matching key_up and release its output keys.
  // manipulator_manager keeps invalid manipulators while active() is true and
  // removes them once active() becomes false.
  validity validity_{validity::valid};
  condition_manager condition_manager_;
};
} // namespace krbn::manipulator::manipulators
