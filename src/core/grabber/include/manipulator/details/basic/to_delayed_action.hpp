#pragma once

class to_delayed_action final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  to_delayed_action(basic& basic,
                    const nlohmann::json& json) : dispatcher_client(),
                                                  basic_(basic),
                                                  current_delayed_action_id_(0) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "to_if_invoked") {
          if (!value.is_array()) {
            logger::get_logger()->error("complex_modifications json error: `to_if_invoked` should be array: {0}", json.dump());
            continue;
          }

          for (const auto& j : value) {
            to_if_invoked_.emplace_back(j);
          }

        } else if (key == "to_if_canceled") {
          if (!value.is_array()) {
            logger::get_logger()->error("complex_modifications json error: `to_if_canceled` should be array: {0}", json.dump());
            continue;
          }

          for (const auto& j : value) {
            to_if_canceled_.emplace_back(j);
          }

        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    } else {
      logger::get_logger()->error("complex_modifications json error: `to_delayed_action` should be object: {0}", json.dump());
    }
  }

  virtual ~to_delayed_action(void) {
    detach_from_dispatcher();
  }

  void setup(const event_queue::entry& front_input_event,
             const std::shared_ptr<manipulated_original_event>& current_manipulated_original_event,
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
    auto duration = time_utility::to_absolute_time_duration(delay_milliseconds);

    enqueue_to_dispatcher(
        [this, delayed_action_id] {
          if (current_delayed_action_id_ != delayed_action_id) {
            return;
          }

          post_events(to_if_invoked_);
        },
        when_now() + time_utility::to_milliseconds(duration));
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

          basic_.post_lazy_modifier_key_events(*front_input_event_,
                                               current_manipulated_original_event_->get_from_mandatory_modifiers(),
                                               event_type::key_up,
                                               time_stamp_delay,
                                               *oeq);

          // Post events

          basic_.post_extra_to_events(*front_input_event_,
                                      events,
                                      *current_manipulated_original_event_,
                                      time_stamp_delay,
                                      *oeq);

          // Restore from_mandatory_modifiers

          basic_.post_lazy_modifier_key_events(*front_input_event_,
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

  basic& basic_;

  std::vector<to_event_definition> to_if_invoked_;
  std::vector<to_event_definition> to_if_canceled_;
  std::optional<event_queue::entry> front_input_event_;
  std::shared_ptr<manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  int current_delayed_action_id_;
};
