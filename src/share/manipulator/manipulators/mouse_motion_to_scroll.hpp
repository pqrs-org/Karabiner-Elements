#pragma once

#include "../types.hpp"
#include "base.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
class mouse_motion_to_scroll final : public base, public pqrs::dispatcher::extra::dispatcher_client {
public:
  mouse_motion_to_scroll(const nlohmann::json& json,
                         const core_configuration::details::complex_modifications_parameters& parameters) : base(),
                                                                                                            accumulated_x_(0),
                                                                                                            accumulated_y_(0),
                                                                                                            momentum_scroll_timer_(*this) {
    try {
      if (!json.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
      }

      for (const auto& [key, value] : json.items()) {
        if (key == "from") {
          if (!value.is_object()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object, but is `{1}`", key, value.dump()));
          }

          for (const auto& [k, v] : value.items()) {
            if (k == "modifiers") {
              try {
                from_modifiers_definition_ = v.get<from_modifiers_definition>();
              } catch (const pqrs::json::unmarshal_error& e) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` error: {2}", key, k, e.what()));
              }
            }
          }
        }
      }

    } catch (...) {
      detach_from_dispatcher();
      throw;
    }
  }

  virtual ~mouse_motion_to_scroll(void) {
    detach_from_dispatcher([this] {
      momentum_scroll_timer_.stop();
    });
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       const std::shared_ptr<event_queue::queue>& output_event_queue,
                                       absolute_time_point now) {
    if (output_event_queue) {
      // ----------------------------------------

      if (!front_input_event.get_valid()) {
        return manipulate_result::passed;
      }

      if (!valid_) {
        return manipulate_result::passed;
      }

      // ----------------------------------------

      auto from_mandatory_modifiers = test_conditions(front_input_event,
                                                      output_event_queue);
      if (!from_mandatory_modifiers) {
        momentum_scroll_timer_.stop();

      } else {
        if (auto pointing_motion = make_pointing_motion(front_input_event)) {
          front_input_event.set_valid(false);

          post_events(*pointing_motion,
                      *from_mandatory_modifiers,
                      front_input_event.get_device_id(),
                      front_input_event.get_event_time_stamp(),
                      front_input_event.get_original_event(),
                      output_event_queue);

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
  std::optional<std::unordered_set<modifier_flag>> test_conditions(const event_queue::entry& front_input_event,
                                                                   const std::shared_ptr<event_queue::queue>& output_event_queue) {
    if (!condition_manager_.is_fulfilled(front_input_event,
                                         output_event_queue->get_manipulator_environment())) {
      return std::nullopt;
    }

    return from_modifiers_definition_.test_modifiers(output_event_queue->get_modifier_flag_manager());
  }

  std::optional<pointing_motion> make_pointing_motion(const event_queue::entry& front_input_event) {
    if (auto m = front_input_event.get_event().find<pointing_motion>()) {
      if (m->get_x() != 0 ||
          m->get_y() != 0) {
        accumulated_x_ += m->get_x();
        accumulated_y_ += m->get_y();

        int divisor = 16;
        int vertical_wheel = accumulated_y_ / divisor;
        int horizontal_wheel = accumulated_x_ / divisor;
        accumulated_x_ -= horizontal_wheel * divisor;
        accumulated_y_ -= vertical_wheel * divisor;

        return pointing_motion(0, 0, -vertical_wheel, horizontal_wheel);
      }
    }

    return std::nullopt;
  }

  void post_events(const pointing_motion& pointing_motion,
                   const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
                   device_id device_id,
                   const event_queue::event_time_stamp& event_time_stamp,
                   const event_queue::event& original_event,
                   std::shared_ptr<event_queue::queue> output_event_queue) {
    if (output_event_queue) {
      absolute_time_duration time_stamp_delay(0);

      // Post from_mandatory_modifiers key_up

      base::post_lazy_modifier_key_events(from_mandatory_modifiers,
                                          event_type::key_up,
                                          device_id,
                                          event_time_stamp,
                                          time_stamp_delay,
                                          original_event,
                                          *output_event_queue);

      // Post new event

      {
        auto t = event_time_stamp;
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue->emplace_back_entry(device_id,
                                               t,
                                               event_queue::event(pointing_motion),
                                               event_type::single,
                                               original_event);
      }

      // Post from_mandatory_modifiers key_down

      base::post_lazy_modifier_key_events(from_mandatory_modifiers,
                                          event_type::key_down,
                                          device_id,
                                          event_time_stamp,
                                          time_stamp_delay,
                                          original_event,
                                          *output_event_queue);
    }
  }

  from_modifiers_definition from_modifiers_definition_;
  int accumulated_x_;
  int accumulated_y_;
  pqrs::dispatcher::extra::timer momentum_scroll_timer_;
};
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
