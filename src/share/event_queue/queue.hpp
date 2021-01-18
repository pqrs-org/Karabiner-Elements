#pragma once

#include "event_queue/entry.hpp"
#include "event_queue/event.hpp"
#include "event_queue/event_time_stamp.hpp"
#include "modifier_flag_manager.hpp"
#include "pointing_button_manager.hpp"
#include <string_view>

namespace krbn {
namespace event_queue {
class queue {
public:
  queue(const queue&) = delete;

  queue(void) : time_stamp_delay_(0) {
  }

  queue(const std::string_view& name)
      : name_(name) {
  }

  const std::string& get_name(void) const {
    return name_;
  }

  void emplace_back_entry(device_id device_id,
                          const event_time_stamp& event_time_stamp,
                          const class event& event,
                          event_type event_type,
                          const class event& original_event,
                          state state,
                          bool lazy = false,
                          validity validity = validity::valid) {
    auto t = event_time_stamp;
    t.set_time_stamp(t.get_time_stamp() + time_stamp_delay_);

    events_.emplace_back(device_id,
                         t,
                         event,
                         event_type,
                         original_event,
                         state,
                         lazy,
                         validity);

    sort_events();

    //
    // Update modifier_flag_manager, pointing_button_manager
    //

    if (auto e = event.get_if<momentary_switch_event>()) {
      if (auto modifier_flag = e->make_modifier_flag()) {
        auto type = (event_type == event_type::key_down ? modifier_flag_manager::active_modifier_flag::type::increase
                                                        : modifier_flag_manager::active_modifier_flag::type::decrease);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         *modifier_flag,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }

      if (e->pointing_button()) {
        auto type = (event_type == event_type::key_down ? pointing_button_manager::active_pointing_button::type::increase
                                                        : pointing_button_manager::active_pointing_button::type::decrease);
        pointing_button_manager::active_pointing_button active_pointing_button(type,
                                                                               e->get_usage_pair(),
                                                                               device_id);
        pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
      }

      // Erase sticky modifiers
      if (event_type == event_type::key_down &&
          validity == validity::valid &&
          !e->modifier_flag()) {
        modifier_flag_manager_.erase_all_sticky_modifier_flags();
      }

    } else if (event.get_type() == event::type::sticky_modifier) {
      if (event_type == event_type::key_down || event_type == event_type::single) {
        if (auto sticky_modifier = event.get_sticky_modifier()) {
          auto type = modifier_flag_manager::active_modifier_flag::type::increase_sticky;

          if (sticky_modifier->second == sticky_modifier_type::toggle) {
            if (modifier_flag_manager_.sticky_size(sticky_modifier->first) > 0) {
              type = modifier_flag_manager::active_modifier_flag::type::decrease_sticky;
            }
          } else {
            if (sticky_modifier->second != sticky_modifier_type::on) {
              type = modifier_flag_manager::active_modifier_flag::type::decrease_sticky;
            }
          }

          modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                           sticky_modifier->first,
                                                                           device_id);
          modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
        }
      }

    } else if (event.get_type() == event::type::caps_lock_state_changed) {
      if (auto integer_value = event.get_integer_value()) {
        auto type = (*integer_value ? modifier_flag_manager::active_modifier_flag::type::increase_led_lock
                                    : modifier_flag_manager::active_modifier_flag::type::decrease_led_lock);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         modifier_flag::caps_lock,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }
    }

    //
    // Update manipulator_environment
    //

    if (event.get_type() == event::type::device_grabbed) {
      if (auto v = event.get_if<device_properties>()) {
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
    if (event_type == event_type::key_down) {
      if (auto set_variable = event.get_set_variable()) {
        manipulator_environment_.set_variable(set_variable->first,
                                              set_variable->second);
      }
    }
    if (auto properties = event.get_if<pqrs::osx::system_preferences::properties>()) {
      manipulator_environment_.set_system_preferences_properties(*properties);
    }
    if (auto configuration = event.get_if<core_configuration::details::virtual_hid_keyboard>()) {
      manipulator_environment_.set_virtual_hid_keyboard_country_code(configuration->get_country_code());
    }
  }

  void push_back_entry(const entry& entry) {
    emplace_back_entry(entry.get_device_id(),
                       entry.get_event_time_stamp(),
                       entry.get_event(),
                       entry.get_event_type(),
                       entry.get_original_event(),
                       entry.get_state(),
                       entry.get_lazy(),
                       entry.get_validity());
  }

  void clear_events(void) {
    events_.clear();
    time_stamp_delay_ = absolute_time_duration(0);
  }

  entry& get_front_event(void) {
    return events_.front();
  }

  void erase_front_event(void) {
    events_.erase(std::begin(events_));
    if (events_.empty()) {
      time_stamp_delay_ = absolute_time_duration(0);
    }
  }

  bool empty(void) const {
    return events_.empty();
  }

  const std::vector<entry>& get_entries(void) const {
    return events_;
  }

  const modifier_flag_manager& get_modifier_flag_manager(void) const {
    return modifier_flag_manager_;
  }

  modifier_flag_manager& get_modifier_flag_manager(void) {
    return const_cast<modifier_flag_manager&>(static_cast<const queue&>(*this).get_modifier_flag_manager());
  }

  void erase_all_active_modifier_flags_except_lock_and_sticky(device_id device_id) {
    modifier_flag_manager_.erase_all_active_modifier_flags_except_lock_and_sticky(device_id);
  }

  void erase_all_active_modifier_flags(device_id device_id) {
    modifier_flag_manager_.erase_all_active_modifier_flags(device_id);
  }

  const pointing_button_manager& get_pointing_button_manager(void) const {
    return pointing_button_manager_;
  }

  void erase_all_active_pointing_buttons_except_lock(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons_except_lock(device_id);
  }

  void erase_all_active_pointing_buttons(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons(device_id);
  }

  const manipulator::manipulator_environment& get_manipulator_environment(void) const {
    return manipulator_environment_;
  }

  void enable_manipulator_environment_json_output(const std::string& file_path) {
    manipulator_environment_.enable_json_output(file_path);
  }

  void disable_manipulator_environment_json_output(void) {
    manipulator_environment_.disable_json_output();
  }

  absolute_time_duration get_time_stamp_delay(void) const {
    return time_stamp_delay_;
  }

  void increase_time_stamp_delay(absolute_time_duration value) {
    time_stamp_delay_ += value;
  }

  static bool needs_swap(const entry& v1, const entry& v2) {
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

    if (v1.get_event_time_stamp().get_time_stamp() == v2.get_event_time_stamp().get_time_stamp()) {
      auto e1 = v1.get_event().get_if<momentary_switch_event>();
      auto e2 = v2.get_event().get_if<momentary_switch_event>();

      if (e1 && e2) {
        auto modifier_flag1 = e1->make_modifier_flag();
        auto modifier_flag2 = e2->make_modifier_flag();

        // If either modifier_flag1 or modifier_flag2 is modifier, reorder it before.

        if (!modifier_flag1 && modifier_flag2) {
          // v2 is modifier_flag
          if (v2.get_event_type() == event_type::key_up) {
            return false;
          } else {
            // reorder to v2,v1 if v2 is pressed.
            return true;
          }
        }

        if (modifier_flag1 && !modifier_flag2) {
          // v1 is modifier_flag
          if (v1.get_event_type() == event_type::key_up) {
            // reorder to v2,v1 if v1 is released.
            return true;
          } else {
            return false;
          }
        }
      }
    }

    return false;
  }

private:
  void sort_events(void) {
    for (size_t i = 0; i < events_.size() - 1;) {
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

  std::string name_;
  std::vector<entry> events_;
  modifier_flag_manager modifier_flag_manager_;
  pointing_button_manager pointing_button_manager_;
  manipulator::manipulator_environment manipulator_environment_;
  absolute_time_duration time_stamp_delay_;
};
} // namespace event_queue
} // namespace krbn
