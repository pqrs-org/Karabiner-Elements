#pragma once

#include "../../types.hpp"
#include "event_sender.hpp"
#include <pqrs/json.hpp>
#include <unordered_set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
class to_if_held_down final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  to_if_held_down(const nlohmann::json& json) : dispatcher_client(),
                                                current_held_down_id_(0) {
    try {
      if (json.is_object()) {
        to_ = std::vector<to_event_definition>{
            json.get<to_event_definition>(),
        };

      } else if (json.is_array()) {
        to_ = json.get<std::vector<to_event_definition>>();

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object or array, but is `{0}`", json.dump()));
      }

    } catch (...) {
      detach_from_dispatcher();
      throw;
    }
  }

  virtual ~to_if_held_down(void) {
    detach_from_dispatcher();
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void setup(const event_queue::entry& front_input_event,
             std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event,
             std::weak_ptr<event_queue::queue> output_event_queue,
             std::chrono::milliseconds threshold_milliseconds) {
    ++current_held_down_id_;

    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    if (to_.empty()) {
      return;
    }

    front_input_event_ = front_input_event;
    current_manipulated_original_event_ = current_manipulated_original_event;
    output_event_queue_ = output_event_queue;

    auto held_down_id = current_held_down_id_;
    auto duration = pqrs::osx::chrono::make_absolute_time_duration(threshold_milliseconds);

    enqueue_to_dispatcher(
        [this, held_down_id] {
          if (current_held_down_id_ != held_down_id) {
            return;
          }

          if (front_input_event_) {
            if (auto oeq = output_event_queue_.lock()) {
              if (auto cmoe = current_manipulated_original_event_.lock()) {
                absolute_time_duration time_stamp_delay(0);

                // ----------------------------------------
                // Post events_at_key_up_

                event_sender::post_events_at_key_up(*front_input_event_,
                                                    *cmoe,
                                                    time_stamp_delay,
                                                    *oeq);

                event_sender::post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                                     *cmoe,
                                                                     time_stamp_delay,
                                                                     *oeq);

                // ----------------------------------------
                // Post to_

                event_sender::post_from_mandatory_modifiers_key_up(*front_input_event_,
                                                                   *cmoe,
                                                                   time_stamp_delay,
                                                                   *oeq);

                event_sender::post_events_at_key_down(*front_input_event_,
                                                      to_,
                                                      *cmoe,
                                                      time_stamp_delay,
                                                      *oeq);

                if (!event_sender::is_last_to_event_modifier_key_event(to_)) {
                  event_sender::post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                                       *cmoe,
                                                                       time_stamp_delay,
                                                                       *oeq);
                }

                // ----------------------------------------

                krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
              }
            }
          }
        },
        when_now() + pqrs::osx::chrono::make_milliseconds(duration));
  }

  void cancel(const event_queue::entry& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    ++current_held_down_id_;
  }

  bool needs_virtual_hid_pointing(void) const {
    return std::any_of(std::begin(to_),
                       std::end(to_),
                       [](auto& e) {
                         return e.needs_virtual_hid_pointing();
                       });
  }

private:
  std::vector<to_event_definition> to_;
  std::optional<event_queue::entry> front_input_event_;
  std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  int current_held_down_id_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
