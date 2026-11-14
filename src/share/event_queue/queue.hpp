#pragma once

#include "event_queue/entry.hpp"
#include "event_queue/event.hpp"
#include "event_queue/event_time_stamp.hpp"
#include "event_queue/modifier_state_manager.hpp"
#include "manipulator/condition_expression_manager.hpp"
#include "pointing_button_manager.hpp"
#include <string_view>

namespace krbn::event_queue {
class queue {
public:
  queue(const queue&) = delete;

  queue() = default;

  void emplace_back_entry(entry::parameters parameters) {
    parameters.event_time_stamp.set_time_stamp(parameters.event_time_stamp.get_time_stamp() + time_stamp_delay_);

    events_.emplace_back(parameters);

    apply_modifier_state(events_.back());

    const auto& event = parameters.event;
    auto event_type = parameters.event_type;
    auto device_id = parameters.device_id;

    //
    // Update pointing_button_manager
    //

    if (auto e = event.get_if<momentary_switch_event>()) {
      if (e->pointing_button()) {
        auto type = (event_type == event_type::key_down ? pointing_button_manager::active_pointing_button::type::increase
                                                        : pointing_button_manager::active_pointing_button::type::decrease);
        pointing_button_manager::active_pointing_button active_pointing_button(type,
                                                                               e->get_usage_pair(),
                                                                               device_id);
        pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
      }
    }

    //
    // Update manipulator_environment
    //

    if (event.get_type() == event::type::device_grabbed) {
      if (auto v = event.get_if<pqrs::not_null_shared_ptr_t<device_properties>>()) {
        manipulator_environment_.insert_device_properties(device_id, *v);
      }
    }
    if (event.get_type() == event::type::device_ungrabbed) {
      manipulator_environment_.erase_device_properties(device_id);
    }
    if (auto frontmost_application = event.get_frontmost_application()) {
      manipulator_environment_.set_frontmost_application(*frontmost_application);
    }
    if (auto properties = event.get_input_source_properties()) {
      manipulator_environment_.set_input_source_properties(*properties);
    }
    if (auto set_variable = event.get_set_variable()) {
      switch (event_type) {
        case event_type::key_down:
          if (auto n = set_variable->get_name()) {
            switch (set_variable->get_type()) {
              case manipulator_environment_variable_set_variable::type::set:
                if (auto v = set_variable->get_value()) {
                  manipulator_environment_.set_variable(*n, *v);
                }
                if (const auto& v = set_variable->get_expression()) {
                  manipulator_environment_.set_variable_system_now_milliseconds();
                  manipulator_environment_.apply_to_expression_variable(*v);

                  manipulator_environment_.set_variable(*n,
                                                        manipulator_environment_variable_value(v->get_expression_wrapper().value<int64_t>()));
                }
                break;

              case manipulator_environment_variable_set_variable::type::unset:
                manipulator_environment_.unset_variable(*n);
                break;
            }
          }
          break;
        case event_type::key_up:
          if (auto n = set_variable->get_name()) {
            switch (set_variable->get_type()) {
              case manipulator_environment_variable_set_variable::type::set:
                if (auto v = set_variable->get_key_up_value()) {
                  manipulator_environment_.set_variable(*n, *v);
                }
                if (const auto& v = set_variable->get_key_up_expression()) {
                  manipulator_environment_.set_variable_system_now_milliseconds();
                  manipulator_environment_.apply_to_expression_variable(*v);

                  manipulator_environment_.set_variable(*n,
                                                        manipulator_environment_variable_value(v->get_expression_wrapper().value<int64_t>()));
                }
                break;
              case manipulator_environment_variable_set_variable::type::unset:
                // Do nothing
                break;
            }
          }
          break;
        case event_type::single:
          // Do nothing
          break;
      }
    }
    if (auto properties = event.get_if<pqrs::osx::system_preferences::properties>()) {
      manipulator_environment_.set_variable("system.scroll_direction_is_natural",
                                            manipulator_environment_variable_value(properties->get_scroll_direction_is_natural()));
      manipulator_environment_.set_variable("system.use_fkeys_as_standard_function_keys",
                                            manipulator_environment_variable_value(properties->get_use_fkeys_as_standard_function_keys()));
    }
    if (auto state = event.get_if<virtual_hid_devices_state>()) {
      manipulator_environment_.set_virtual_hid_devices_state(*state);
    }
  }

  void push_back_entry(const entry& entry) {
    emplace_back_entry({
        .device_id = entry.get_device_id(),
        .event_time_stamp = entry.get_event_time_stamp(),
        .event = entry.get_event(),
        .event_type = entry.get_event_type(),
        .event_integer_value = entry.get_event_integer_value(),
        .original_event = entry.get_original_event(),
        .state = entry.get_state(),
        .lazy = entry.get_lazy(),
        .validity = entry.get_validity(),
        .output_only_modifier = entry.get_output_only_modifier(),
    });
  }

  void clear_events() {
    events_.clear();
    time_stamp_delay_ = absolute_time_duration(0);
  }

  [[nodiscard]] entry& get_front_event() {
    return events_.front();
  }

  void erase_front_event() {
    events_.erase(std::begin(events_));
    if (events_.empty()) {
      time_stamp_delay_ = absolute_time_duration(0);
    }
  }

  [[nodiscard]] bool empty() const {
    return events_.empty();
  }

  [[nodiscard]] const std::vector<entry>& get_entries() const {
    return events_;
  }

  [[nodiscard]] const modifier_flag_manager& get_output_modifier_flag_manager() const {
    return modifier_state_manager_.get_output_state();
  }

  [[nodiscard]] modifier_flag_manager& get_output_modifier_flag_manager() {
    return modifier_state_manager_.get_output_state();
  }

  [[nodiscard]] const modifier_flag_manager& get_regular_matching_modifier_flag_manager() const {
    return modifier_state_manager_.get_regular_matching_state();
  }

  [[nodiscard]] std::unordered_set<modifier_flag> make_modifier_flags_for_matching(const entry& entry) const {
    return modifier_state_manager_.make_modifier_flags_for_matching(entry);
  }

  [[nodiscard]] std::unordered_set<modifier_flag> make_output_only_modifier_flags_for_manipulation(const entry& entry) const {
    return modifier_state_manager_.make_output_only_modifier_flags_for_manipulation(entry);
  }

  // Apply the modifier portion of an entry without enqueueing it.
  //
  // A deferred key_up event can be posted directly to a downstream queue. In
  // that case, the queues it skipped must still update their modifier state so
  // future output does not restore a modifier that has already been released.
  void apply_modifier_state(const entry& entry) {
    modifier_state_manager_.apply(entry);
  }

  void erase_all_active_modifier_flags_except_lock_and_sticky(device_id device_id) {
    modifier_state_manager_.erase_all_active_modifier_flags_except_lock_and_sticky(device_id);
  }

  void erase_all_active_modifier_flags(device_id device_id) {
    modifier_state_manager_.erase_all_active_modifier_flags(device_id);
  }

  [[nodiscard]] const pointing_button_manager& get_pointing_button_manager() const {
    return pointing_button_manager_;
  }

  void erase_all_active_pointing_buttons_except_lock(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons_except_lock(device_id);
  }

  void erase_all_active_pointing_buttons(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons(device_id);
  }

  [[nodiscard]] const manipulator::manipulator_environment& get_manipulator_environment() const {
    return manipulator_environment_;
  }

  [[nodiscard]] manipulator::manipulator_environment& get_manipulator_environment() {
    return const_cast<manipulator::manipulator_environment&>(static_cast<const queue&>(*this).get_manipulator_environment());
  }

  [[nodiscard]] absolute_time_duration get_time_stamp_delay() const {
    return time_stamp_delay_;
  }

  void increase_time_stamp_delay(absolute_time_duration value) {
    time_stamp_delay_ += value;
  }

  void sort_events() {
    if (events_.empty()) {
      return;
    }

    for (int i = 0; i < events_.size() - 1;) {
      if (needs_swap(events_[i], events_[i + 1])) {
        std::swap(events_[i], events_[i + 1]);
        if (i > 0) {
          --i;
        }
        continue;
      }
      ++i;
    }
  }

  [[nodiscard]] static bool needs_swap(const entry& v1, const entry& v2) {
    // Some devices are send modifier flag and key at the same HID report.
    // For example, a key sends control+up-arrow by this reports.
    //
    //   modifiers: 0x0
    //   keys: 0x0 0x0 0x0 0x0 0x0 0x0
    //
    //   modifiers: 0x1
    //   keys: 0x52 0x0 0x0 0x0 0x0 0x0
    //
    // In this case, macOS does not guarantee the value event order to be modifier first.
    // At least macOS 10.12 or prior sends the up-arrow event first.
    //
    //   ----------------------------------------
    //   Example of hid value events in a single queue at control+up-arrow
    //
    //   1. up-arrow keydown
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:1
    //
    //   2. control keydown
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:1
    //
    //   3. up-arrow keyup
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:0
    //
    //   4. control keyup
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:0
    //   ----------------------------------------
    //
    // These events will not be interpreted as intended in this order.
    // Thus, we have to reorder the events.
    //
    // Devices that exhibit this behavior are:
    // - Logitech Keyboard K480 (the expose key)
    // - RollerMouse (the copy key and the paste key)
    //
    // Some devices also release a modifier flag in the HID report that presses another key.
    // For example, QMK will release shift in the same report as the pressed key while
    // also reporting the press of another key (e.g., slash). For example, with KC_TILDE
    // and KC_SLASH:
    //
    //   modifiers: 0x2
    //   keys: 0x35 0x0 0x0 0x0 0x0 0x0
    //
    //   modifiers: 0x0
    //   keys: 0x35 0x38 0x0 0x0 0x0 0x0
    //
    // Observed on an ErgoDox, running v25 of QMK.
    // macOS sends the slash keydown before the shift keyup here too, and the slash would be shifted.
    // Thus, a modifier keyup is reordered after released keys, but before pressed keys.

    // Timestamps from different devices do not establish an ordering between
    // their events. Reorder only events that belong to the same device.
    if (v1.get_device_id() == v2.get_device_id() &&
        v1.get_event_time_stamp().get_time_stamp() == v2.get_event_time_stamp().get_time_stamp()) {
      auto e1 = v1.get_event().get_if<momentary_switch_event>();
      auto e2 = v2.get_event().get_if<momentary_switch_event>();

      if (e1 && e2) {
        auto modifier_flag1 = e1->make_modifier_flag();
        auto modifier_flag2 = e2->make_modifier_flag();

        // If either modifier_flag1 or modifier_flag2 is modifier, reorder it before.

        if (!modifier_flag1 && modifier_flag2) {
          // v2 is modifier_flag
          if (v2.get_event_type() == event_type::key_up) {
            // reorder to v2,v1 if v2 is released and v1 is pressed.
            return v1.get_event_type() == event_type::key_down;
          } else {
            // reorder to v2,v1 if v2 is pressed.
            return true;
          }
        }

        if (modifier_flag1 && !modifier_flag2) {
          // v1 is modifier_flag
          if (v1.get_event_type() == event_type::key_up) {
            // reorder to v2,v1 if v1 is released, unless v2 is pressed.
            return v2.get_event_type() != event_type::key_down;
          } else {
            return false;
          }
        }
      }
    }

    return false;
  }

private:
  std::vector<entry> events_;

  // Keeps output and matching modifier states synchronized for this queue.
  modifier_state_manager modifier_state_manager_;

  pointing_button_manager pointing_button_manager_;
  manipulator::manipulator_environment manipulator_environment_;
  absolute_time_duration time_stamp_delay_{0};
};
} // namespace krbn::event_queue
