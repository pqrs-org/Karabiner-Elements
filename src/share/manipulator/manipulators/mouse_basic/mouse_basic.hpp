#pragma once

#include "../../types.hpp"
#include "../base.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_basic {
class mouse_basic final : public base {
public:
  mouse_basic(const nlohmann::json& json,
              gsl::not_null<std::shared_ptr<const core_configuration::details::complex_modifications_parameters>> parameters)
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
        discard_horizontal_wheel_(false) {
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

      } else if (key == "description" ||
                 key == "conditions" ||
                 key == "parameters" ||
                 key == "type") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }
  }

  virtual ~mouse_basic(void) {
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       std::shared_ptr<event_queue::queue> output_event_queue,
                                       absolute_time_point now) {
    if (output_event_queue) {
      //
      // Determine whether to skip
      //

      if (front_input_event.get_validity() == validity::invalid) {
        return manipulate_result::passed;
      }

      if (validity_ == validity::invalid) {
        return manipulate_result::passed;
      }

      if (!condition_manager_.is_fulfilled(front_input_event,
                                           output_event_queue->get_manipulator_environment())) {
        return manipulate_result::passed;
      }

      //
      // Manipulate
      //

      if (auto m = front_input_event.get_event().get_if<pointing_motion>()) {
        if (flip_x_ ||
            flip_y_ ||
            flip_vertical_wheel_ ||
            flip_horizontal_wheel_ ||
            swap_xy_ ||
            swap_wheels_ ||
            discard_x_ ||
            discard_y_ ||
            discard_vertical_wheel_ ||
            discard_horizontal_wheel_) {
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
                                                 front_input_event.get_original_event(),
                                                 event_queue::state::manipulated,
                                                 front_input_event.get_lazy());

          return manipulate_result::manipulated;
        }
      }
    }

    return manipulate_result::passed;
  }

  virtual bool active(void) const {
    return false;
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return true;
  }

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) {
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) {
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) {
  }

private:
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
};
} // namespace mouse_basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
