#pragma once

#include "../../types.hpp"
#include "../base.hpp"
#include "counter.hpp"
#include "krbn_notification_center.hpp"
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
                                                                                                            dispatcher_client() {
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

            } else {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: unknown key `{1}` in `{2}`",
                                                            key,
                                                            k,
                                                            value.dump()));
            }
          }

        } else if (key == "options") {
          if (!value.is_object()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object, but is `{1}`", key, value.dump()));
          }

          for (const auto& [k, v] : value.items()) {
            if (k == "momentum_scroll_enabled") {
              if (!v.is_boolean()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be boolean, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_momentum_scroll_enabled(v.get<bool>());
            } else if (k == "speed_multiplier") {
              if (!v.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be number, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_speed_multiplier(v.get<double>());
            } else if (k == "threshold") { // (secret parameter)
              if (!v.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be number, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_threshold(v.get<int>());
            } else if (k == "recent_time_duration_milliseconds") { // (secret parameter)
              if (!v.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be number, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_recent_time_duration_milliseconds(
                  std::chrono::milliseconds(v.get<int>()));
            } else if (k == "direction_lock_threshold") { // (secret parameter)
              if (!v.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be number, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_direction_lock_threshold(v.get<int>());
            } else if (k == "scroll_event_interval_milliseconds_threshold") { // (secret parameter)
              if (!v.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}.{1}` must be number, but is `{2}`", key, k, v.dump()));
              }

              counter_parameters_.set_scroll_event_interval_milliseconds_threshold(
                  std::chrono::milliseconds(v.get<int>()));
            }
          }

        } else if (key == "description" ||
                   key == "conditions" ||
                   key == "parameters" ||
                   key == "type") {
          // Do nothing

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
        }
      }

      counter_ = std::make_unique<counter>(weak_dispatcher_,
                                           parameters,
                                           counter_parameters_);

      counter_->scroll_event_arrived.connect([this](auto&& pointing_motion) {
        post_events(pointing_motion);
      });

    } catch (...) {
      detach_from_dispatcher([this] {
        counter_ = nullptr;
      });
      throw;
    }
  }

  virtual ~mouse_motion_to_scroll(void) {
    detach_from_dispatcher([this] {
      counter_ = nullptr;
    });
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       std::shared_ptr<event_queue::queue> output_event_queue,
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
        counter_->async_reset();

      } else {
        if (auto m = front_input_event.get_event().find<pointing_motion>()) {
          front_input_event.set_valid(false);

          counter_->update(*m, when_now());

          from_mandatory_modifiers_ = from_mandatory_modifiers;
          device_id_ = front_input_event.get_device_id();
          original_event_ = front_input_event.get_original_event();
          weak_output_event_queue_ = output_event_queue;

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
  std::shared_ptr<std::unordered_set<modifier_flag>> test_conditions(const event_queue::entry& front_input_event,
                                                                     std::shared_ptr<event_queue::queue> output_event_queue) const {
    if (!condition_manager_.is_fulfilled(front_input_event,
                                         output_event_queue->get_manipulator_environment())) {
      return nullptr;
    }

    return from_modifiers_definition_.test_modifiers(output_event_queue->get_modifier_flag_manager());
  }

  void post_events(const pointing_motion& pointing_motion) {
    if (auto output_event_queue = weak_output_event_queue_.lock()) {
      event_queue::event_time_stamp event_time_stamp(pqrs::osx::chrono::mach_absolute_time_point());
      absolute_time_duration time_stamp_delay(0);

      // Post from_mandatory_modifiers key_up

      if (from_mandatory_modifiers_) {
        base::post_lazy_modifier_key_events(*from_mandatory_modifiers_,
                                            event_type::key_up,
                                            device_id_,
                                            event_time_stamp,
                                            time_stamp_delay,
                                            original_event_,
                                            *output_event_queue);
      }

      // Post new event

      {
        auto t = event_time_stamp;
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        output_event_queue->emplace_back_entry(device_id_,
                                               t,
                                               event_queue::event(pointing_motion),
                                               event_type::single,
                                               original_event_);
      }

      // Post from_mandatory_modifiers key_down

      if (from_mandatory_modifiers_) {
        base::post_lazy_modifier_key_events(*from_mandatory_modifiers_,
                                            event_type::key_down,
                                            device_id_,
                                            event_time_stamp,
                                            time_stamp_delay,
                                            original_event_,
                                            *output_event_queue);
      }

      krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
    }
  }

  from_modifiers_definition from_modifiers_definition_;
  counter_parameters counter_parameters_;
  std::unique_ptr<counter> counter_;

  std::shared_ptr<std::unordered_set<modifier_flag>> from_mandatory_modifiers_;
  device_id device_id_;
  event_queue::event original_event_;
  std::weak_ptr<event_queue::queue> weak_output_event_queue_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
