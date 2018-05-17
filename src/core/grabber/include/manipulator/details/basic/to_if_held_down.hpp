#pragma once

class to_if_held_down final {
public:
  to_if_held_down(basic& basic,
                  const nlohmann::json& json) : basic_(basic) {
    if (json.is_array()) {
      for (const auto& j : json) {
        to_.emplace_back(j);
      }
    } else {
      logger::get_logger().error("complex_modifications json error: `to_if_held_down` should be object: {0}", json.dump());
    }
  }

  void setup(const event_queue::queued_event& front_input_event,
             const std::shared_ptr<manipulated_original_event>& current_manipulated_original_event,
             const std::shared_ptr<event_queue>& output_event_queue,
             int threshold_milliseconds) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      manipulator_timer_id_ = boost::none;
      return;
    }

    if (to_.empty()) {
      return;
    }

    front_input_event_ = front_input_event;
    current_manipulated_original_event_ = current_manipulated_original_event;
    output_event_queue_ = output_event_queue;

    auto when = front_input_event.get_event_time_stamp().get_time_stamp() + time_utility::nano_to_absolute(threshold_milliseconds * NSEC_PER_MSEC);
    manipulator_timer_id_ = manipulator_timer::get_instance().add_entry(when);
  }

  void cancel(const event_queue::queued_event& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    if (!manipulator_timer_id_) {
      return;
    }

    manipulator_timer_id_ = boost::none;
  }

  void manipulator_timer_invoked(manipulator_timer::timer_id timer_id, uint64_t now) {
    if (timer_id == manipulator_timer_id_) {
      manipulator_timer_id_ = boost::none;

      if (front_input_event_) {
        if (auto oeq = output_event_queue_.lock()) {
          if (auto cmoe = current_manipulated_original_event_.lock()) {
            uint64_t time_stamp_delay = 0;

            // ----------------------------------------
            // Post events_at_key_up_

            basic_.post_events_at_key_up(*front_input_event_,
                                         *cmoe,
                                         time_stamp_delay,
                                         *oeq);

            basic_.post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                          *cmoe,
                                                          time_stamp_delay,
                                                          *oeq);

            // ----------------------------------------
            // Post to_

            basic_.post_from_mandatory_modifiers_key_up(*front_input_event_,
                                                        *cmoe,
                                                        time_stamp_delay,
                                                        *oeq);

            basic_.post_events_at_key_down(*front_input_event_,
                                           to_,
                                           *cmoe,
                                           time_stamp_delay,
                                           *oeq);

            if (!basic_.is_last_to_event_modifier_key_event(to_)) {
              basic_.post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                            *cmoe,
                                                            time_stamp_delay,
                                                            *oeq);
            }

            // ----------------------------------------

            krbn_notification_center::get_instance().input_event_arrived();
          }
        }
      }
    }
  }

  bool needs_virtual_hid_pointing(void) const {
    return std::any_of(std::begin(to_),
                       std::end(to_),
                       [](auto& e) {
                         return e.needs_virtual_hid_pointing();
                       });
  }

private:
  basic& basic_;
  std::vector<to_event_definition> to_;
  boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
  boost::optional<event_queue::queued_event> front_input_event_;
  std::weak_ptr<manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue> output_event_queue_;
};
