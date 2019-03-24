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
                         const core_configuration::details::complex_modifications_parameters& parameters) : base() {
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
    detach_from_dispatcher();
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

      if (auto new_event = make_new_event(front_input_event, output_event_queue)) {
        front_input_event.set_valid(false);

        output_event_queue->emplace_back_entry(front_input_event.get_device_id(),
                                               front_input_event.get_event_time_stamp(),
                                               *new_event,
                                               event_type::single,
                                               front_input_event.get_original_event());

        return manipulate_result::manipulated;
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
  std::optional<event_queue::event> make_new_event(const event_queue::entry& front_input_event,
                                                   const std::shared_ptr<event_queue::queue>& output_event_queue) const {
    if (output_event_queue) {
      if (auto m = front_input_event.get_event().find<pointing_motion>()) {
        if (m->get_x() != 0 ||
            m->get_y() != 0) {
          // Check mandatory_modifiers and conditions

          if (auto modifiers = from_modifiers_definition_.test_modifiers(output_event_queue->get_modifier_flag_manager())) {
            // from_mandatory_modifiers = *modifiers;
          } else {
            return std::nullopt;
          }

          if (!condition_manager_.is_fulfilled(front_input_event,
                                               output_event_queue->get_manipulator_environment())) {
            return std::nullopt;
          }

          return event_queue::event(pointing_motion(0, 0, m->get_x(), m->get_y()));
        }
      }
    }

    return std::nullopt;
  }

  from_modifiers_definition from_modifiers_definition_;
};
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
