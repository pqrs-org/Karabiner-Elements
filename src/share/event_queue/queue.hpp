#pragma once

#include "event_queue/entry.hpp"
#include "event_queue/event.hpp"
#include "event_queue/event_time_stamp.hpp"
#include "hash_utility.hpp"
#include "modifier_flag_manager.hpp"
#include "pointing_button_manager.hpp"

namespace krbn {
namespace event_queue {
class queue {
public:
  queue(const queue&) = delete;

  queue(void) : time_stamp_delay_(0) {
  }

  void emplace_back_event(device_id device_id,
                          const event_time_stamp& event_time_stamp,
                          const class event& event,
                          event_type event_type,
                          const class event& original_event,
                          bool lazy = false) {
    auto t = event_time_stamp;
    t.set_time_stamp(t.get_time_stamp() + time_stamp_delay_);

    events_.emplace_back(device_id,
                         t,
                         event,
                         event_type,
                         original_event,
                         lazy);

    sort_events();

    // Update modifier_flag_manager

    if (auto key_code = event.get_key_code()) {
      if (auto modifier_flag = types::make_modifier_flag(*key_code)) {
        auto type = (event_type == event_type::key_down ? modifier_flag_manager::active_modifier_flag::type::increase
                                                        : modifier_flag_manager::active_modifier_flag::type::decrease);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         *modifier_flag,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }
    }

    if (event.get_type() == event::type::caps_lock_state_changed) {
      if (auto integer_value = event.get_integer_value()) {
        auto type = (*integer_value ? modifier_flag_manager::active_modifier_flag::type::increase_lock
                                    : modifier_flag_manager::active_modifier_flag::type::decrease_lock);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         modifier_flag::caps_lock,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }
    }

    // Update pointing_button_manager

    if (auto pointing_button = event.get_pointing_button()) {
      if (*pointing_button != pointing_button::zero) {
        auto type = (event_type == event_type::key_down ? pointing_button_manager::active_pointing_button::type::increase
                                                        : pointing_button_manager::active_pointing_button::type::decrease);
        pointing_button_manager::active_pointing_button active_pointing_button(type,
                                                                               *pointing_button,
                                                                               device_id);
        pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
      }
    }

    // Update manipulator_environment
    if (auto frontmost_application = event.get_frontmost_application()) {
      manipulator_environment_.set_frontmost_application(*frontmost_application);
    }
    if (auto input_source_identifiers = event.get_input_source_identifiers()) {
      manipulator_environment_.set_input_source_identifiers(*input_source_identifiers);
    }
    if (event_type == event_type::key_down) {
      if (auto set_variable = event.get_set_variable()) {
        manipulator_environment_.set_variable(set_variable->first,
                                              set_variable->second);
      }
    }
    if (auto keyboard_type = event.get_keyboard_type()) {
      manipulator_environment_.set_keyboard_type(*keyboard_type);
    }
  }

  void push_back_event(const entry& entry) {
    emplace_back_event(entry.get_device_id(),
                       entry.get_event_time_stamp(),
                       entry.get_event(),
                       entry.get_event_type(),
                       entry.get_original_event(),
                       entry.get_lazy());
    events_.back().set_valid(entry.get_valid());
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

  void erase_all_active_modifier_flags_except_lock(device_id device_id) {
    modifier_flag_manager_.erase_all_active_modifier_flags_except_lock(device_id);
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

  const manipulator_environment& get_manipulator_environment(void) const {
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
      auto key_code1 = v1.get_event().get_key_code();
      auto key_code2 = v2.get_event().get_key_code();

      if (key_code1 && key_code2) {
        auto modifier_flag1 = types::make_modifier_flag(*key_code1);
        auto modifier_flag2 = types::make_modifier_flag(*key_code2);

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

  static std::vector<std::pair<boost::optional<hid_value>, entry>> make_entries(const std::vector<hid_value>& hid_values,
                                                                                device_id device_id) {
    std::vector<std::pair<boost::optional<hid_value>, entry>> result;

    // The pointing motion usage (hid_usage::gd_x, hid_usage::gd_y, etc.) are splitted from one HID report.
    // We have to join them into one pointing_motion event to avoid VMware Remote Console problem that VMRC ignores frequently events.

    boost::optional<absolute_time> pointing_motion_time_stamp;
    boost::optional<int> pointing_motion_x;
    boost::optional<int> pointing_motion_y;
    boost::optional<int> pointing_motion_vertical_wheel;
    boost::optional<int> pointing_motion_horizontal_wheel;

    auto emplace_back_pointing_motion_event = [&] {
      if (pointing_motion_time_stamp) {
        pointing_motion pointing_motion(
            pointing_motion_x ? *pointing_motion_x : 0,
            pointing_motion_y ? *pointing_motion_y : 0,
            pointing_motion_vertical_wheel ? *pointing_motion_vertical_wheel : 0,
            pointing_motion_horizontal_wheel ? *pointing_motion_horizontal_wheel : 0);

        event_queue::event event(pointing_motion);

        result.emplace_back(boost::none,
                            entry(device_id,
                                  event_time_stamp(*pointing_motion_time_stamp),
                                  event,
                                  event_type::single,
                                  event));

        pointing_motion_time_stamp = boost::none;
        pointing_motion_x = boost::none;
        pointing_motion_y = boost::none;
        pointing_motion_vertical_wheel = boost::none;
        pointing_motion_horizontal_wheel = boost::none;
      }
    };

    for (const auto& v : hid_values) {
      if (auto usage_page = v.get_hid_usage_page()) {
        if (auto usage = v.get_hid_usage()) {
          if (auto key_code = types::make_key_code(*usage_page, *usage)) {
            event_queue::event event(*key_code);
            result.emplace_back(v,
                                entry(device_id,
                                      event_time_stamp(v.get_time_stamp()),
                                      event,
                                      v.get_integer_value() ? event_type::key_down : event_type::key_up,
                                      event));

          } else if (auto consumer_key_code = types::make_consumer_key_code(*usage_page, *usage)) {
            event_queue::event event(*consumer_key_code);
            result.emplace_back(v,
                                entry(device_id,
                                      event_time_stamp(v.get_time_stamp()),
                                      event,
                                      v.get_integer_value() ? event_type::key_down : event_type::key_up,
                                      event));

          } else if (auto pointing_button = types::make_pointing_button(*usage_page, *usage)) {
            event_queue::event event(*pointing_button);
            result.emplace_back(v,
                                entry(device_id,
                                      event_time_stamp(v.get_time_stamp()),
                                      event,
                                      v.get_integer_value() ? event_type::key_down : event_type::key_up,
                                      event));

          } else if (*usage_page == hid_usage_page::generic_desktop &&
                     *usage == hid_usage::gd_x) {
            if (pointing_motion_x) {
              emplace_back_pointing_motion_event();
            }
            pointing_motion_time_stamp = v.get_time_stamp();
            pointing_motion_x = static_cast<int>(v.get_integer_value());

          } else if (*usage_page == hid_usage_page::generic_desktop &&
                     *usage == hid_usage::gd_y) {
            if (pointing_motion_y) {
              emplace_back_pointing_motion_event();
            }
            pointing_motion_time_stamp = v.get_time_stamp();
            pointing_motion_y = static_cast<int>(v.get_integer_value());

          } else if (*usage_page == hid_usage_page::generic_desktop &&
                     *usage == hid_usage::gd_wheel) {
            if (pointing_motion_vertical_wheel) {
              emplace_back_pointing_motion_event();
            }
            pointing_motion_time_stamp = v.get_time_stamp();
            pointing_motion_vertical_wheel = static_cast<int>(v.get_integer_value());

          } else if (*usage_page == hid_usage_page::consumer &&
                     *usage == hid_usage::csmr_acpan) {
            if (pointing_motion_horizontal_wheel) {
              emplace_back_pointing_motion_event();
            }
            pointing_motion_time_stamp = v.get_time_stamp();
            pointing_motion_horizontal_wheel = static_cast<int>(v.get_integer_value());
          } else if (*usage_page == hid_usage_page::leds &&
                     *usage == hid_usage::led_caps_lock) {
            event_queue::event event(event_queue::event::type::caps_lock_state_changed,
                                     v.get_integer_value());
            result.emplace_back(v,
                                entry(device_id,
                                      event_time_stamp(v.get_time_stamp()),
                                      event,
                                      event_type::single,
                                      event));
          }
        }
      }
    }

    emplace_back_pointing_motion_event();

    return result;
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

  std::vector<entry> events_;
  modifier_flag_manager modifier_flag_manager_;
  pointing_button_manager pointing_button_manager_;
  manipulator_environment manipulator_environment_;
  absolute_time_duration time_stamp_delay_;
};
} // namespace event_queue
} // namespace krbn
