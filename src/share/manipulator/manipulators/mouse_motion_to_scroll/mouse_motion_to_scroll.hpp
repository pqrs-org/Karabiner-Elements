#pragma once

#include "../../types.hpp"
#include "../base.hpp"
#include "count_converter.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class mouse_motion_to_scroll final : public base, public pqrs::dispatcher::extra::dispatcher_client {
public:
  mouse_motion_to_scroll(const nlohmann::json& json,
                         const core_configuration::details::complex_modifications_parameters& parameters) : base(),
                                                                                                            x_count_converter_(32),
                                                                                                            y_count_converter_(32),
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

          if (!pointing_motion->is_zero()) {
            auto&& device_id = front_input_event.get_device_id();
            auto&& original_event = front_input_event.get_original_event();

            post_events(*pointing_motion,
                        *from_mandatory_modifiers,
                        device_id,
                        front_input_event.get_event_time_stamp(),
                        original_event,
                        output_event_queue);

            std::weak_ptr<event_queue::queue> weak_output_event_queue(output_event_queue);

            momentum_scroll_timer_.start(
                [this,
                 from_mandatory_modifiers,
                 device_id,
                 original_event,
                 weak_output_event_queue] {
                  auto pointing_motion = make_momentum_scroll_pointing_motion();
                  if (pointing_motion.is_zero()) {
                    momentum_scroll_timer_.stop();

                  } else {
                    if (auto output_event_queue = weak_output_event_queue.lock()) {
                      event_queue::event_time_stamp t(pqrs::osx::chrono::mach_absolute_time_point());

                      post_events(pointing_motion,
                                  *from_mandatory_modifiers,
                                  device_id,
                                  t,
                                  original_event,
                                  output_event_queue);
                    }
                  }
                },
                std::chrono::milliseconds(20));
          }

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
                                                                   const std::shared_ptr<event_queue::queue>& output_event_queue) const {
    if (!condition_manager_.is_fulfilled(front_input_event,
                                         output_event_queue->get_manipulator_environment())) {
      return std::nullopt;
    }

    return from_modifiers_definition_.test_modifiers(output_event_queue->get_modifier_flag_manager());
  }

  std::optional<pointing_motion> make_pointing_motion(const event_queue::entry& front_input_event) {
    if (auto m = front_input_event.get_event().find<pointing_motion>()) {
      auto x = x_count_converter_.update(m->get_x());
      auto y = y_count_converter_.update(m->get_y());

      return pointing_motion(0, 0, -y, x);
    }

    return std::nullopt;
  }

  pointing_motion make_momentum_scroll_pointing_motion(void) {
    auto x = x_count_converter_.update_momentum();
    auto y = y_count_converter_.update_momentum();
    return pointing_motion(0, 0, -y, x);
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

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    }
  }

  from_modifiers_definition from_modifiers_definition_;
  count_converter x_count_converter_;
  count_converter y_count_converter_;
  pqrs::dispatcher::extra::timer momentum_scroll_timer_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
