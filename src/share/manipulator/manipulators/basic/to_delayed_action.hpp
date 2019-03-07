#pragma once

#include "../../types.hpp"
#include "event_sender.hpp"
#include <unordered_set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
class to_delayed_action final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  to_delayed_action(const nlohmann::json& json) : dispatcher_client(),
                                                  current_delayed_action_id_(0) {
    try {
      if (!json.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
      }

      for (const auto& [key, value] : json.items()) {
        if (key == "to_if_invoked") {
          if (value.is_object()) {
            try {
              to_if_invoked_ = std::vector<to_event_definition>{
                  value.get<to_event_definition>(),
              };
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
            }

          } else if (value.is_array()) {
            try {
              to_if_invoked_ = value.get<std::vector<to_event_definition>>();
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, value.dump()));
          }

        } else if (key == "to_if_canceled") {
          if (value.is_object()) {
            try {
              to_if_canceled_ = std::vector<to_event_definition>{
                  value.get<to_event_definition>(),
              };
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
            }

          } else if (value.is_array()) {
            try {
              to_if_canceled_ = value.get<std::vector<to_event_definition>>();
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, value.dump()));
          }

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
        }
      }

    } catch (...) {
      detach_from_dispatcher();
      throw;
    }
  }

  virtual ~to_delayed_action(void) {
    detach_from_dispatcher();
  }

  const std::vector<to_event_definition>& get_to_if_invoked(void) const {
    return to_if_invoked_;
  }

  const std::vector<to_event_definition>& get_to_if_canceled(void) const {
    return to_if_canceled_;
  }

  void setup(const event_queue::entry& front_input_event,
             const std::shared_ptr<manipulated_original_event::manipulated_original_event>& current_manipulated_original_event,
             const std::shared_ptr<event_queue::queue>& output_event_queue,
             std::chrono::milliseconds delay_milliseconds) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    if (to_if_invoked_.empty() &&
        to_if_canceled_.empty()) {
      return;
    }

    ++current_delayed_action_id_;

    front_input_event_ = front_input_event;
    current_manipulated_original_event_ = current_manipulated_original_event;
    output_event_queue_ = output_event_queue;

    auto delayed_action_id = current_delayed_action_id_;
    auto duration = pqrs::osx::chrono::make_absolute_time_duration(delay_milliseconds);

    enqueue_to_dispatcher(
        [this, delayed_action_id] {
          if (current_delayed_action_id_ != delayed_action_id) {
            return;
          }

          post_events(to_if_invoked_);
        },
        when_now() + pqrs::osx::chrono::make_milliseconds(duration));
  }

  void cancel(const event_queue::entry& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    ++current_delayed_action_id_;

    post_events(to_if_canceled_);
  }

  bool needs_virtual_hid_pointing(void) const {
    for (const auto& events : {to_if_invoked_,
                               to_if_canceled_}) {
      if (std::any_of(std::begin(events),
                      std::end(events),
                      [](auto& e) {
                        return e.needs_virtual_hid_pointing();
                      })) {
        return true;
      }
    }
    return false;
  }

private:
  void post_events(const std::vector<to_event_definition>& events) {
    if (front_input_event_) {
      if (current_manipulated_original_event_) {
        if (auto oeq = output_event_queue_.lock()) {
          absolute_time_duration time_stamp_delay(0);

          // Release from_mandatory_modifiers

          event_sender::post_lazy_modifier_key_events(*front_input_event_,
                                                      current_manipulated_original_event_->get_from_mandatory_modifiers(),
                                                      event_type::key_up,
                                                      time_stamp_delay,
                                                      *oeq);

          // Post events

          event_sender::post_extra_to_events(*front_input_event_,
                                             events,
                                             *current_manipulated_original_event_,
                                             time_stamp_delay,
                                             *oeq);

          // Restore from_mandatory_modifiers

          event_sender::post_lazy_modifier_key_events(*front_input_event_,
                                                      current_manipulated_original_event_->get_from_mandatory_modifiers(),
                                                      event_type::key_down,
                                                      time_stamp_delay,
                                                      *oeq);

          krbn_notification_center::get_instance().enqueue_input_event_arrived(*this);
        }
      }
    }

    current_manipulated_original_event_ = nullptr;
  }

  std::vector<to_event_definition> to_if_invoked_;
  std::vector<to_event_definition> to_if_canceled_;
  std::optional<event_queue::entry> front_input_event_;
  std::shared_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  int current_delayed_action_id_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
