#pragma once

#include "device_properties.hpp"
#include "hat_switch_convert.hpp"
#include "pressed_keys_manager.hpp"
#include "queue.hpp"
#include <gsl/gsl>

namespace krbn {
namespace event_queue {
namespace utility {

static inline int adjust_pointing_motion_value(const pqrs::osx::iokit_hid_value& value,
                                               double multiplier) {
  auto integer_value = value.get_integer_value();
  auto v = static_cast<int>(static_cast<double>(integer_value * multiplier));

  if (v == 0) {
    // To prevent all adjusted values from becoming 0 when an extremely small value is specified for the multiplier,
    // ensure that the value is at least 1 or -1.
    if (integer_value > 0) {
      return 1;
    } else if (integer_value < 0) {
      return -1;
    }
  }

  return std::min(127, std::max(-127, v));
}

struct make_entries_parameters final {
  double pointing_motion_xy_multiplier = 1.0;
  double pointing_motion_wheels_multiplier = 1.0;
};

static inline not_null_entries_ptr_t make_entries(gsl::not_null<std::shared_ptr<device_properties>> device_properties,
                                                  const std::vector<pqrs::osx::iokit_hid_value>& hid_values,
                                                  const make_entries_parameters& parameters) {
  auto result = std::make_shared<std::vector<not_null_const_entry_ptr_t>>();

  // The pointing motion usage (hid_usage::gd_x, hid_usage::gd_y, etc.) are splitted from one HID report.
  // We have to join them into one pointing_motion event to avoid VMware Remote Console problem that VMRC ignores frequently events.

  std::optional<absolute_time_point> pointing_motion_time_stamp;
  std::optional<int> pointing_motion_x;
  std::optional<int> pointing_motion_y;
  std::optional<int> pointing_motion_vertical_wheel;
  std::optional<int> pointing_motion_horizontal_wheel;

  auto emplace_back_pointing_motion_event = [&] {
    if (pointing_motion_time_stamp) {
      pointing_motion pointing_motion(
          pointing_motion_x ? *pointing_motion_x : 0,
          pointing_motion_y ? *pointing_motion_y : 0,
          pointing_motion_vertical_wheel ? *pointing_motion_vertical_wheel : 0,
          pointing_motion_horizontal_wheel ? *pointing_motion_horizontal_wheel : 0);

      event_queue::event event(pointing_motion);
      auto e = std::make_shared<entry>(device_properties->get_device_id(),
                                       event_time_stamp(*pointing_motion_time_stamp),
                                       event,
                                       event_type::single,
                                       event,
                                       state::original);
      result->push_back(e);

      pointing_motion_time_stamp = std::nullopt;
      pointing_motion_x = std::nullopt;
      pointing_motion_y = std::nullopt;
      pointing_motion_vertical_wheel = std::nullopt;
      pointing_motion_horizontal_wheel = std::nullopt;
    }
  };

  //
  // In game pad, the following usage is handled by game_pad_stick_convert and is not handled here.
  //
  // - generic_desktop::x
  // - generic_desktop::y
  // - generic_desktop::z
  // - generic_desktop::rz
  //
  bool is_game_pad = device_properties->get_device_identifiers().get_is_game_pad();

  for (const auto& v : hid_values) {
    if (auto usage_page = v.get_usage_page()) {
      if (auto usage = v.get_usage()) {
        if (momentary_switch_event::target(*usage_page, *usage)) {
          event_queue::event event(momentary_switch_event(*usage_page, *usage));
          auto e = std::make_shared<entry>(device_properties->get_device_id(),
                                           event_time_stamp(v.get_time_stamp()),
                                           event,
                                           v.get_integer_value() ? event_type::key_down : event_type::key_up,
                                           event,
                                           state::original);
          result->push_back(e);

        } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                 pqrs::hid::usage::generic_desktop::x) &&
                   !is_game_pad) {
          if (pointing_motion_x) {
            emplace_back_pointing_motion_event();
          }
          pointing_motion_time_stamp = v.get_time_stamp();
          pointing_motion_x = adjust_pointing_motion_value(v,
                                                           parameters.pointing_motion_xy_multiplier);

        } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                 pqrs::hid::usage::generic_desktop::y) &&
                   !is_game_pad) {
          if (pointing_motion_y) {
            emplace_back_pointing_motion_event();
          }
          pointing_motion_time_stamp = v.get_time_stamp();
          pointing_motion_y = adjust_pointing_motion_value(v,
                                                           parameters.pointing_motion_xy_multiplier);

        } else if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                                 pqrs::hid::usage::generic_desktop::wheel)) {
          if (pointing_motion_vertical_wheel) {
            emplace_back_pointing_motion_event();
          }
          pointing_motion_time_stamp = v.get_time_stamp();
          pointing_motion_vertical_wheel = adjust_pointing_motion_value(v,
                                                                        parameters.pointing_motion_wheels_multiplier);

        } else if (v.conforms_to(pqrs::hid::usage_page::consumer,
                                 pqrs::hid::usage::consumer::ac_pan)) {
          if (pointing_motion_horizontal_wheel) {
            emplace_back_pointing_motion_event();
          }
          pointing_motion_time_stamp = v.get_time_stamp();
          pointing_motion_horizontal_wheel = adjust_pointing_motion_value(v,
                                                                          parameters.pointing_motion_wheels_multiplier);

        } else if (v.conforms_to(pqrs::hid::usage_page::leds,
                                 pqrs::hid::usage::led::caps_lock)) {
          auto event = event_queue::event::make_caps_lock_state_changed_event(v.get_integer_value());
          auto e = std::make_shared<entry>(device_properties->get_device_id(),
                                           event_time_stamp(v.get_time_stamp()),
                                           event,
                                           event_type::single,
                                           event,
                                           state::virtual_event);
          result->push_back(e);

        } else if (is_game_pad) {
          if (v.conforms_to(pqrs::hid::usage_page::generic_desktop,
                            pqrs::hid::usage::generic_desktop::hat_switch)) {
            // Convert hat switch to dpad.
            auto pairs = hat_switch_converter::get_global_hat_switch_converter()->to_dpad_events(device_properties->get_device_id(),
                                                                                                 v.get_integer_value());
            for (const auto& pair : pairs) {
              event_queue::event event(pair.first);
              auto e = std::make_shared<entry>(device_properties->get_device_id(),
                                               event_time_stamp(v.get_time_stamp()),
                                               event,
                                               pair.second,
                                               event,
                                               state::original);
              result->push_back(e);
            }
          }
        }
      }
    }
  }

  emplace_back_pointing_motion_event();

  return result;
}

static inline not_null_entries_ptr_t insert_device_keys_and_pointing_buttons_are_released_event(not_null_entries_ptr_t entries,
                                                                                                device_id device_id,
                                                                                                gsl::not_null<std::shared_ptr<pressed_keys_manager>> pressed_keys_manager) {
  auto result = std::make_shared<std::vector<not_null_const_entry_ptr_t>>();

  for (const auto& entry : *entries) {
    result->push_back(entry);

    if (entry->get_device_id() == device_id) {
      if (entry->get_event_type() == event_type::key_down) {
        if (auto e = entry->get_event().get_if<momentary_switch_event>()) {
          pressed_keys_manager->insert(*e);
        }
      } else if (entry->get_event_type() == event_type::key_up) {
        if (!pressed_keys_manager->empty()) {
          if (auto e = entry->get_event().get_if<momentary_switch_event>()) {
            pressed_keys_manager->erase(*e);
          }

          if (pressed_keys_manager->empty()) {
            auto event = event::make_device_keys_and_pointing_buttons_are_released_event();
            auto e = std::make_shared<event_queue::entry>(device_id,
                                                          entry->get_event_time_stamp(),
                                                          event,
                                                          event_type::single,
                                                          event,
                                                          state::virtual_event);
            result->push_back(e);
          }
        }
      }
    }
  }

  return result;
}

} // namespace utility
} // namespace event_queue
} // namespace krbn
