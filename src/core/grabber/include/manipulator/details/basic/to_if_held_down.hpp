#pragma once

class to_if_held_down final {
public:
  to_if_held_down(basic& basic,
                  const nlohmann::json& json) : basic_(basic),
                                                manipulator_object_id_(make_new_manipulator_object_id()) {
    if (json.is_array()) {
      for (const auto& j : json) {
        to_.emplace_back(j);
      }
    } else {
      logger::get_logger().error("complex_modifications json error: `to_if_held_down` should be object: {0}", json.dump());
    }

    if (auto manipulator_dispatcher = basic_.get_weak_manipulator_dispatcher().lock()) {
      manipulator_dispatcher->async_attach(manipulator_object_id_);
    }
    if (auto manipulator_timer = basic_.get_weak_manipulator_timer().lock()) {
      manipulator_timer->async_attach(manipulator_object_id_);
    }
  }

  ~to_if_held_down(void) {
    if (auto manipulator_timer = basic_.get_weak_manipulator_timer().lock()) {
      manipulator_timer->detach(manipulator_object_id_);
    }

    if (auto manipulator_dispatcher = basic_.get_weak_manipulator_dispatcher().lock()) {
      manipulator_dispatcher->detach(manipulator_object_id_);
    }
  }

  void async_setup(const event_queue::entry& front_input_event,
                   std::weak_ptr<manipulated_original_event> current_manipulated_original_event,
                   std::weak_ptr<event_queue> output_event_queue,
                   std::chrono::milliseconds threshold_milliseconds) {
    if (auto manipulator_dispatcher = basic_.get_weak_manipulator_dispatcher().lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this, front_input_event, current_manipulated_original_event, output_event_queue, threshold_milliseconds] {
            if (auto manipulator_timer = basic_.get_weak_manipulator_timer().lock()) {
              if (front_input_event.get_event_type() != event_type::key_down) {
                manipulator_timer->async_erase(manipulator_object_id_);
                return;
              }

              if (to_.empty()) {
                return;
              }

              front_input_event_ = front_input_event;
              current_manipulated_original_event_ = current_manipulated_original_event;
              output_event_queue_ = output_event_queue;

              auto time_stamp = front_input_event.get_event_time_stamp().get_time_stamp();
              auto when = time_stamp + time_utility::to_absolute_time(threshold_milliseconds);

              manipulator_timer->enqueue(
                  manipulator_object_id_,
                  [this] {
                    if (front_input_event_) {
                      if (auto oeq = output_event_queue_.lock()) {
                        if (auto cmoe = current_manipulated_original_event_.lock()) {
                          absolute_time time_stamp_delay(0);

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
                  },
                  when);
              manipulator_timer->async_invoke(time_stamp);
            }
          });
    }
  }

  void async_cancel(const event_queue::entry& front_input_event) {
    if (auto manipulator_dispatcher = basic_.get_weak_manipulator_dispatcher().lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this, front_input_event] {
            if (front_input_event.get_event_type() != event_type::key_down) {
              return;
            }

            if (auto manipulator_timer = basic_.get_weak_manipulator_timer().lock()) {
              manipulator_timer->async_erase(manipulator_object_id_);
            }
          });
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
  manipulator_object_id manipulator_object_id_;
  boost::optional<event_queue::entry> front_input_event_;
  std::weak_ptr<manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue> output_event_queue_;
};
