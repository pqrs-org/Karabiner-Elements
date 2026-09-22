#pragma once

#include "../../types.hpp"
#include "../base.hpp"

namespace krbn::manipulator::manipulators::mouse_basic {
class mouse_basic final : public base {
public:
  mouse_basic(const nlohmann::json& json,
              pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters)
      : base(),
        flip_x_(false),
        flip_y_(false),
        flip_vertical_wheel_(false),
        flip_horizontal_wheel_(false),
        swap_xy_(false),
        swap_wheels_(false),
        discard_x_(false),
        discard_y_(false),
        discard_vertical_wheel_(false),
        discard_horizontal_wheel_(false),
        to_buttons_horizontal_wheel_(false),
        enabled_(false),
        active_horizontal_wheel_button_(std::nullopt) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      if (key == "flip") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          pqrs::json::requires_string(j, "items in `" + key + "`");

          if (j == "x") {
            flip_x_ = true;
          } else if (j == "y") {
            flip_y_ = true;
          } else if (j == "vertical_wheel") {
            flip_vertical_wheel_ = true;
          } else if (j == "horizontal_wheel") {
            flip_horizontal_wheel_ = true;
          }
        }

      } else if (key == "swap") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          pqrs::json::requires_string(j, "items in `" + key + "`");

          if (j == "xy") {
            swap_xy_ = true;
          } else if (j == "wheels") {
            swap_wheels_ = true;
          }
        }

      } else if (key == "discard") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          pqrs::json::requires_string(j, "items in `" + key + "`");

          if (j == "x") {
            discard_x_ = true;
          } else if (j == "y") {
            discard_y_ = true;
          } else if (j == "vertical_wheel") {
            discard_vertical_wheel_ = true;
          } else if (j == "horizontal_wheel") {
            discard_horizontal_wheel_ = true;
          }
        }

      } else if (key == "to_buttons") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          pqrs::json::requires_string(j, "items in `" + key + "`");

          if (j == "horizontal_wheel") {
            to_buttons_horizontal_wheel_ = true;
          }
        }

      } else if (key == "description" ||
                 key == "conditions" ||
                 key == "parameters" ||
                 key == "type") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }

    enabled_ = flip_x_ ||
               flip_y_ ||
               flip_vertical_wheel_ ||
               flip_horizontal_wheel_ ||
               swap_xy_ ||
               swap_wheels_ ||
               discard_x_ ||
               discard_y_ ||
               discard_vertical_wheel_ ||
               discard_horizontal_wheel_ ||
               to_buttons_horizontal_wheel_;
  }

  ~mouse_basic() override {
  }

  bool already_manipulated(const event_queue::entry& front_input_event) override {
    return false;
  }

  manipulate_result manipulate(event_queue::entry& front_input_event,
                               const event_queue::queue& input_event_queue,
                               std::shared_ptr<event_queue::queue> output_event_queue,
                               absolute_time_point now) override {
    if (output_event_queue) {
      //
      // Determine whether to skip
      //

      if (front_input_event.get_validity() == validity::invalid) {
        return manipulate_result::passed;
      }

      if (validity_ == validity::invalid) {
        // A config reload superseded this manipulator. Stop starting new `to_buttons`
        // holds, but still release one we're already holding so it isn't left stuck
        // on the virtual device once this (now-unreachable) instance is discarded.
        release_horizontal_wheel_button_if_needed(front_input_event, output_event_queue);
        return manipulate_result::passed;
      }

      manipulator::conditions::condition_context condition_context{
          .device_id = front_input_event.get_device_id(),
          .state = front_input_event.get_state(),
      };
      if (!condition_manager_.is_fulfilled(condition_context,
                                           output_event_queue->get_manipulator_environment())) {
        return manipulate_result::passed;
      }

      //
      // Manipulate
      //

      if (auto m = front_input_event.get_event().get_if<pointing_motion>()) {
        if (enabled_) {
          front_input_event.set_validity(validity::invalid);

          auto motion = *m;

          //
          // When swap and flip are used simultaneously, it is more natural for humans to apply swap first.
          //

          if (swap_xy_) {
            auto x = motion.get_x();
            auto y = motion.get_y();
            motion.set_x(y);
            motion.set_y(x);
          }

          if (swap_wheels_) {
            auto v = motion.get_vertical_wheel();
            auto h = motion.get_horizontal_wheel();
            motion.set_vertical_wheel(h);
            motion.set_horizontal_wheel(v);
          }

          if (flip_x_) {
            motion.set_x(-motion.get_x());
          }

          if (flip_y_) {
            motion.set_y(-motion.get_y());
          }

          if (flip_vertical_wheel_) {
            motion.set_vertical_wheel(-motion.get_vertical_wheel());
          }

          if (flip_horizontal_wheel_) {
            motion.set_horizontal_wheel(-motion.get_horizontal_wheel());
          }

          // Convert horizontal wheel motion (a tilt-wheel switch) into a button hold instead
          // of scroll motion, using button30/button31 since Karabiner-DriverKit-VirtualHIDPointing
          // only reports buttons 1-32 (pointing_button_manager.hpp).

          std::optional<pqrs::hid::usage::value_t> new_horizontal_wheel_button;
          absolute_time_duration horizontal_wheel_button_time_stamp_delay(0);

          if (to_buttons_horizontal_wheel_) {
            if (motion.get_horizontal_wheel() < 0) {
              new_horizontal_wheel_button = pqrs::hid::usage::button::button_30;
            } else if (motion.get_horizontal_wheel() > 0) {
              new_horizontal_wheel_button = pqrs::hid::usage::button::button_31;
            }

            motion.set_horizontal_wheel(0);
          }

          if (discard_x_) {
            motion.set_x(0);
          }

          if (discard_y_) {
            motion.set_y(0);
          }

          if (discard_vertical_wheel_) {
            motion.set_vertical_wheel(0);
          }

          if (discard_horizontal_wheel_) {
            motion.set_horizontal_wheel(0);
          }

          output_event_queue->emplace_back_entry(front_input_event.get_device_id(),
                                                 front_input_event.get_event_time_stamp(),
                                                 event_queue::event(motion),
                                                 front_input_event.get_event_type(),
                                                 front_input_event.get_event_integer_value(),
                                                 front_input_event.get_original_event(),
                                                 event_queue::state::manipulated,
                                                 front_input_event.get_lazy());

          if (new_horizontal_wheel_button != active_horizontal_wheel_button_) {
            // `manipulator::manipulators::basic` correlates a key_up with its key_down by
            // comparing (device_id, event, original_event); real hardware buttons match
            // because original_event is the button event itself. Follow the same
            // self-referential convention so our down (from one pointing_motion tick) and
            // up (from a later, different tick) are still recognized as a pair.
            auto emit_button_event = [&](pqrs::hid::usage::value_t button, event_type type, int value) {
              auto button_event = event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button, button));
              auto t = front_input_event.get_event_time_stamp();
              t.set_time_stamp(t.get_time_stamp() + horizontal_wheel_button_time_stamp_delay++);
              output_event_queue->emplace_back_entry(front_input_event.get_device_id(),
                                                     t,
                                                     button_event,
                                                     type,
                                                     event_integer_value::value_t(value),
                                                     button_event,
                                                     event_queue::state::manipulated);
            };

            if (active_horizontal_wheel_button_) {
              emit_button_event(*active_horizontal_wheel_button_, event_type::key_up, 0);
            }

            if (new_horizontal_wheel_button) {
              emit_button_event(*new_horizontal_wheel_button, event_type::key_down, 1);
            }

            active_horizontal_wheel_button_ = new_horizontal_wheel_button;
          }

          return manipulate_result::manipulated;
        }
      }
    }

    return manipulate_result::passed;
  }

  bool active() const override {
    return active_horizontal_wheel_button_ != std::nullopt;
  }

  bool needs_virtual_hid_pointing() const override {
    return true;
  }

  void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                  event_queue::queue& output_event_queue) override {
    // The manager already erased active pointing buttons from `output_event_queue` before
    // calling this. Keep our own held-button state in sync so a future tilt is not silently
    // ignored because we still believe the button is down.
    active_horizontal_wheel_button_ = std::nullopt;
  }

  void handle_device_ungrabbed_event(device_id device_id,
                                     const event_queue::queue& output_event_queue,
                                     absolute_time_point time_stamp) override {
    active_horizontal_wheel_button_ = std::nullopt;
  }

  void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                   event_queue::queue& output_event_queue) override {
  }

private:
  void release_horizontal_wheel_button_if_needed(const event_queue::entry& front_input_event,
                                                 std::shared_ptr<event_queue::queue> output_event_queue) {
    if (!active_horizontal_wheel_button_) {
      return;
    }

    auto m = front_input_event.get_event().get_if<pointing_motion>();
    if (!m) {
      return;
    }

    // `swap_wheels_` is the only one of our transforms that can move a nonzero value out of
    // (or a zero value into) `horizontal_wheel`, so it's the only one we need to account for
    // here to tell whether the switch has actually been released.
    auto horizontal_wheel = swap_wheels_ ? m->get_vertical_wheel() : m->get_horizontal_wheel();
    if (horizontal_wheel != 0) {
      return;
    }

    auto button_event = event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                  *active_horizontal_wheel_button_));
    output_event_queue->emplace_back_entry(front_input_event.get_device_id(),
                                           front_input_event.get_event_time_stamp(),
                                           button_event,
                                           event_type::key_up,
                                           event_integer_value::value_t(0),
                                           button_event,
                                           event_queue::state::manipulated);

    active_horizontal_wheel_button_ = std::nullopt;
  }

  bool flip_x_;
  bool flip_y_;
  bool flip_vertical_wheel_;
  bool flip_horizontal_wheel_;
  bool swap_xy_;
  bool swap_wheels_;
  bool discard_x_;
  bool discard_y_;
  bool discard_vertical_wheel_;
  bool discard_horizontal_wheel_;
  bool to_buttons_horizontal_wheel_;
  bool enabled_;
  std::optional<pqrs::hid::usage::value_t> active_horizontal_wheel_button_;
};
} // namespace krbn::manipulator::manipulators::mouse_basic
