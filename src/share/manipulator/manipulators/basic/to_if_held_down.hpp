#pragma once

#include "../../types.hpp"
#include "event_sender.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/json.hpp>
#include <unordered_set>
#include <vector>

namespace krbn::manipulator::manipulators::basic {
class to_if_held_down final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  to_if_held_down(const nlohmann::json& json) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
          if (json.is_object()) {
            to_.push_back(std::make_shared<to_event_definition>(json));

          } else if (json.is_array()) {
            for (const auto& j : json) {
              to_.push_back(std::make_shared<to_event_definition>(j));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("json must be object or array, but is `{0}`", pqrs::json::dump_for_error_message(json)));
          }
        });
  }

  ~to_if_held_down() override {
    detach_from_dispatcher();
  }

  [[nodiscard]] const to_event_definitions& get_to() const {
    return to_;
  }

  void setup(const event_queue::entry& front_input_event,
             std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event,
             std::weak_ptr<event_queue::queue> output_event_queue,
             std::shared_ptr<const manipulators::deferred_output_session> deferred_output_session,
             std::chrono::milliseconds threshold_milliseconds) {
    held_down_task_.cancel();

    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    if (to_.empty()) {
      return;
    }

    front_input_event_ = front_input_event;
    current_manipulated_original_event_ = current_manipulated_original_event;
    output_event_queue_ = output_event_queue;
    deferred_output_session_ = deferred_output_session;

    auto duration = pqrs::osx::chrono::make_absolute_time_duration(threshold_milliseconds);

    held_down_task_.debounce_after(
        [this] {
          if (front_input_event_) {
            if (auto oeq = output_event_queue_.lock()) {
              if (auto cmoe = current_manipulated_original_event_.lock()) {
                absolute_time_duration time_stamp_delay(0);
                const auto condition_context = manipulator::conditions::make_device_and_state_condition_context(*front_input_event_);

                // ----------------------------------------
                // Post events_at_key_up_

                event_sender::post_events_at_key_up(*cmoe,
                                                    front_input_event_->get_event_time_stamp(),
                                                    time_stamp_delay,
                                                    *oeq);

                event_sender::post_from_mandatory_modifiers_key_down(*cmoe,
                                                                     front_input_event_->get_device_id(),
                                                                     front_input_event_->get_event_time_stamp(),
                                                                     time_stamp_delay,
                                                                     front_input_event_->get_original_event(),
                                                                     *oeq);

                // ----------------------------------------
                // Post to_

                event_sender::post_output(
                    to_,
                    condition_context,
                    *cmoe,
                    deferred_output_session_,
                    *front_input_event_,
                    *oeq,
                    [&](event_queue::queue& queue) {
                      event_sender::post_from_mandatory_modifiers_key_up(*cmoe,
                                                                         front_input_event_->get_device_id(),
                                                                         front_input_event_->get_event_time_stamp(),
                                                                         time_stamp_delay,
                                                                         front_input_event_->get_original_event(),
                                                                         queue);

                      auto cancellation = event_sender::post_events_at_key_down(to_,
                                                                                *cmoe,
                                                                                condition_context,
                                                                                front_input_event_->get_device_id(),
                                                                                front_input_event_->get_event_time_stamp(),
                                                                                time_stamp_delay,
                                                                                front_input_event_->get_original_event(),
                                                                                queue);

                      if (cancellation == event_sender::to_if_alone_cancellation::required) {
                        krbn_notification_center::get_instance().enqueue_to_if_alone_cancellation_requested(*this,
                                                                                                            *front_input_event_);
                      }

                      if (!event_sender::is_last_to_event_modifier_key_event(to_)) {
                        event_sender::post_from_mandatory_modifiers_key_down(*cmoe,
                                                                             front_input_event_->get_device_id(),
                                                                             front_input_event_->get_event_time_stamp(),
                                                                             time_stamp_delay,
                                                                             front_input_event_->get_original_event(),
                                                                             queue);
                      }
                    });

                // ----------------------------------------

                krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
              }
            }
          }

          deferred_output_session_ = nullptr;
        },
        pqrs::osx::chrono::make_milliseconds(duration));
  }

  void cancel(const event_queue::entry& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    held_down_task_.cancel();
    deferred_output_session_ = nullptr;
  }

  [[nodiscard]] bool needs_virtual_hid_pointing() const {
    return manipulator::any_event_definition_matches(
        [](const auto& definition) {
          return definition.needs_virtual_hid_pointing();
        },
        to_);
  }

  [[nodiscard]] bool uses_caps_lock_led_override() const {
    return manipulator::any_event_definition_matches(
        [](const auto& definition) {
          return definition.uses_caps_lock_led_override();
        },
        to_);
  }

private:
  to_event_definitions to_;
  std::optional<event_queue::entry> front_input_event_;
  std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  std::shared_ptr<const manipulators::deferred_output_session> deferred_output_session_;

  // Construct after potentially throwing members; destruction requires detach.
  pqrs::dispatcher::extra::debounced_task held_down_task_{*this};
};
} // namespace krbn::manipulator::manipulators::basic
