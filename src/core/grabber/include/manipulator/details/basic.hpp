#pragma once

#include "krbn_notification_center.hpp"
#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "time_utility.hpp"
#include <json/json.hpp>
#include <unordered_set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
class basic final : public base {
public:
  class manipulated_original_event final {
  public:
    class events_at_key_up final {
    public:
      class entry {
      public:
        entry(const event_queue::queued_event::event event,
              event_type event_type,
              bool lazy) : event_(event),
                           event_type_(event_type),
                           lazy_(lazy) {
        }

        const event_queue::queued_event::event& get_event(void) const {
          return event_;
        }

        event_type get_event_type(void) const {
          return event_type_;
        }

        bool get_lazy(void) const {
          return lazy_;
        }

      private:
        event_queue::queued_event::event event_;
        event_type event_type_;
        bool lazy_;
      };

      const std::vector<entry>& get_events(void) const {
        return events_;
      }

      void emplace_back_event(const event_queue::queued_event::event event,
                              event_type event_type,
                              bool lazy) {
        events_.emplace_back(event,
                             event_type,
                             lazy);
      }

      void clear_events(void) {
        events_.clear();
      }

    private:
      std::vector<entry> events_;
    };

    manipulated_original_event(device_id device_id,
                               const event_queue::queued_event::event& original_event,
                               const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
                               uint64_t key_down_time_stamp) : device_id_(device_id),
                                                               original_event_(original_event),
                                                               from_mandatory_modifiers_(from_mandatory_modifiers),
                                                               key_down_time_stamp_(key_down_time_stamp),
                                                               alone_(true) {
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    const event_queue::queued_event::event& get_original_event(void) const {
      return original_event_;
    }

    const std::unordered_set<modifier_flag>& get_from_mandatory_modifiers(void) const {
      return from_mandatory_modifiers_;
    }

    uint64_t get_key_down_time_stamp(void) const {
      return key_down_time_stamp_;
    }

    bool get_alone(void) const {
      return alone_;
    }

    const events_at_key_up& get_events_at_key_up(void) const {
      return events_at_key_up_;
    }
    events_at_key_up& get_events_at_key_up(void) {
      return const_cast<events_at_key_up&>(static_cast<const manipulated_original_event&>(*this).get_events_at_key_up());
    }

    void unset_alone(void) {
      alone_ = false;
    }

    bool operator==(const manipulated_original_event& other) const {
      // Do not compare `from_mandatory_modifiers_`.
      return get_device_id() == other.get_device_id() &&
             get_original_event() == other.get_original_event();
    }

  private:
    device_id device_id_;
    event_queue::queued_event::event original_event_;
    std::unordered_set<modifier_flag> from_mandatory_modifiers_;
    uint64_t key_down_time_stamp_;
    bool alone_;
    events_at_key_up events_at_key_up_;
  };

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

      auto when = front_input_event.get_time_stamp() + time_utility::nano_to_absolute(threshold_milliseconds * NSEC_PER_MSEC);
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

    void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
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

              // ----------------------------------------
              // Post to_

              for (auto it = std::begin(to_); it != std::end(to_); std::advance(it, 1)) {
                if (auto event = it->to_event()) {
                  auto to_modifier_events = it->make_modifier_events();

                  for (const auto& e : to_modifier_events) {
                    oeq->emplace_back_event(front_input_event_->get_device_id(),
                                            front_input_event_->get_time_stamp() + time_stamp_delay++,
                                            e,
                                            event_type::key_down,
                                            front_input_event_->get_original_event(),
                                            true);
                  }

                  oeq->emplace_back_event(front_input_event_->get_device_id(),
                                          front_input_event_->get_time_stamp() + time_stamp_delay++,
                                          *event,
                                          event_type::key_down,
                                          front_input_event_->get_original_event(),
                                          it->get_lazy());

                  if (it != std::end(to_) - 1 || !it->get_repeat()) {
                    oeq->emplace_back_event(front_input_event_->get_device_id(),
                                            front_input_event_->get_time_stamp() + time_stamp_delay++,
                                            *event,
                                            event_type::key_up,
                                            front_input_event_->get_original_event(),
                                            it->get_lazy());
                  } else {
                    cmoe->get_events_at_key_up().emplace_back_event(*event,
                                                                    event_type::key_up,
                                                                    it->get_lazy());
                  }

                  bool is_modifier_key_event = false;
                  if (auto key_code = event->get_key_code()) {
                    if (types::make_modifier_flag(*key_code) != boost::none) {
                      is_modifier_key_event = true;
                    }
                  }

                  bool lazy = !is_modifier_key_event || it->get_lazy();

                  for (const auto& e : to_modifier_events) {
                    if (it == std::end(to_) - 1 && is_modifier_key_event) {
                      cmoe->get_events_at_key_up().emplace_back_event(e,
                                                                      event_type::key_up,
                                                                      lazy);
                    } else {
                      oeq->emplace_back_event(front_input_event_->get_device_id(),
                                              front_input_event_->get_time_stamp() + time_stamp_delay++,
                                              e,
                                              event_type::key_up,
                                              front_input_event_->get_original_event(),
                                              true);
                    }
                  }
                }
              }

              krbn_notification_center::get_instance().input_event_arrived();
            }
          }
        }
      }
    }

    bool needs_virtual_hid_pointing(void) const {
      for (const auto& e : to_) {
        if (e.needs_virtual_hid_pointing()) {
          return true;
        }
      }
      return false;
    }

  private:
    basic& basic_;
    std::vector<to_event_definition> to_;
    boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
    boost::optional<event_queue::queued_event> front_input_event_;
    std::weak_ptr<manipulated_original_event> current_manipulated_original_event_;
    std::weak_ptr<event_queue> output_event_queue_;
  };

  class to_delayed_action final {
  public:
    to_delayed_action(basic& basic,
                      const nlohmann::json& json) : basic_(basic) {
      if (json.is_object()) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "to_if_invoked") {
            if (!value.is_array()) {
              logger::get_logger().error("complex_modifications json error: `to_if_invoked` should be array: {0}", json.dump());
              continue;
            }

            for (const auto& j : value) {
              to_if_invoked_.emplace_back(j);
            }

          } else if (key == "to_if_canceled") {
            if (!value.is_array()) {
              logger::get_logger().error("complex_modifications json error: `to_if_canceled` should be array: {0}", json.dump());
              continue;
            }

            for (const auto& j : value) {
              to_if_canceled_.emplace_back(j);
            }

          } else {
            logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
          }
        }
      } else {
        logger::get_logger().error("complex_modifications json error: `to_delayed_action` should be object: {0}", json.dump());
      }
    }

    void setup(const event_queue::queued_event& front_input_event,
               const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
               const std::shared_ptr<event_queue>& output_event_queue,
               int delay_milliseconds) {
      if (front_input_event.get_event_type() != event_type::key_down) {
        return;
      }

      if (to_if_invoked_.empty() &&
          to_if_canceled_.empty()) {
        return;
      }

      front_input_event_ = front_input_event;
      from_mandatory_modifiers_ = from_mandatory_modifiers;
      output_event_queue_ = output_event_queue;

      auto when = front_input_event.get_time_stamp() + time_utility::nano_to_absolute(delay_milliseconds * NSEC_PER_MSEC);
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

      post_events(to_if_canceled_);
    }

    void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
      if (timer_id == manipulator_timer_id_) {
        manipulator_timer_id_ = boost::none;
        post_events(to_if_invoked_);
        krbn_notification_center::get_instance().input_event_arrived();
      }
    }

    bool needs_virtual_hid_pointing(void) const {
      for (const auto& events : {to_if_invoked_,
                                 to_if_canceled_}) {
        for (const auto& e : events) {
          if (e.needs_virtual_hid_pointing()) {
            return true;
          }
        }
      }
      return false;
    }

  private:
    void post_events(const std::vector<to_event_definition>& events) const {
      if (front_input_event_) {
        if (auto oeq = output_event_queue_.lock()) {
          uint64_t time_stamp_delay = 0;

          // Release from_mandatory_modifiers

          basic_.post_lazy_modifier_key_events(*front_input_event_,
                                               from_mandatory_modifiers_,
                                               event_type::key_up,
                                               time_stamp_delay,
                                               *oeq);

          // Post events

          basic_.post_extra_to_events(*front_input_event_,
                                      events,
                                      time_stamp_delay,
                                      *oeq);

          // Restore from_mandatory_modifiers

          basic_.post_lazy_modifier_key_events(*front_input_event_,
                                               from_mandatory_modifiers_,
                                               event_type::key_down,
                                               time_stamp_delay,
                                               *oeq);
        }
      }
    }

    basic& basic_;
    std::vector<to_event_definition> to_if_invoked_;
    std::vector<to_event_definition> to_if_canceled_;
    boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
    boost::optional<event_queue::queued_event> front_input_event_;
    std::unordered_set<modifier_flag> from_mandatory_modifiers_;
    std::weak_ptr<event_queue> output_event_queue_;
  };

  basic(const nlohmann::json& json,
        const core_configuration::profile::complex_modifications::parameters& parameters) : base(),
                                                                                            parameters_(parameters),
                                                                                            from_(json_utility::find_copy(json, "from", nlohmann::json())) {
    for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
      // it.key() is always std::string.
      const auto& key = it.key();
      const auto& value = it.value();

      if (key == "to") {
        if (!value.is_array()) {
          logger::get_logger().error("complex_modifications json error: `to` should be array: {0}", json.dump());
          continue;
        }

        for (const auto& j : value) {
          to_.emplace_back(j);
        }

      } else if (key == "to_after_key_up") {
        if (!value.is_array()) {
          logger::get_logger().error("complex_modifications json error: `to_after_key_up` should be array: {0}", json.dump());
          continue;
        }

        for (const auto& j : value) {
          to_after_key_up_.emplace_back(j);
        }

      } else if (key == "to_if_alone") {
        if (!value.is_array()) {
          logger::get_logger().error("complex_modifications json error: `to_if_alone` should be array: {0}", json.dump());
          continue;
        }

        for (const auto& j : value) {
          to_if_alone_.emplace_back(j);
        }

      } else if (key == "to_if_held_down") {
        to_if_held_down_ = std::make_unique<to_if_held_down>(*this, value);

      } else if (key == "to_delayed_action") {
        to_delayed_action_ = std::make_unique<to_delayed_action>(*this, value);

      } else if (key == "description" ||
                 key == "conditions" ||
                 key == "parameters" ||
                 key == "from" ||
                 key == "type") {
        // Do nothing
      } else {
        logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
      }
    }
  }

  basic(const from_event_definition& from,
        const to_event_definition& to) : from_(from),
                                         to_({to}) {
  }

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          const std::shared_ptr<event_queue>& output_event_queue) {
    if (output_event_queue) {
      unset_alone_if_needed(front_input_event.get_event(),
                            front_input_event.get_event_type());

      if (to_if_held_down_) {
        to_if_held_down_->cancel(front_input_event);
      }

      if (to_delayed_action_) {
        to_delayed_action_->cancel(front_input_event);
      }

      // ----------------------------------------

      bool is_target = false;

      if (auto key_code = front_input_event.get_event().get_key_code()) {
        if (from_.get_key_code() == key_code ||
            from_.get_any_type() == event_definition::type::key_code) {
          is_target = true;
        }
      }
      if (auto consumer_key_code = front_input_event.get_event().get_consumer_key_code()) {
        if (from_.get_consumer_key_code() == consumer_key_code ||
            from_.get_any_type() == event_definition::type::consumer_key_code) {
          is_target = true;
        }
      }
      if (auto pointing_button = front_input_event.get_event().get_pointing_button()) {
        if (from_.get_pointing_button() == pointing_button ||
            from_.get_any_type() == event_definition::type::pointing_button) {
          is_target = true;
        }
      }

      if (!front_input_event.get_valid()) {
        return;
      }

      if (is_target) {
        std::shared_ptr<manipulated_original_event> current_manipulated_original_event;

        switch (front_input_event.get_event_type()) {
          case event_type::key_down: {
            std::unordered_set<modifier_flag> from_mandatory_modifiers;

            // ----------------------------------------
            // Check whether event is target.

            if (!valid_) {
              is_target = false;
            }

            if (is_target) {
              if (auto modifiers = from_.test_modifiers(output_event_queue->get_modifier_flag_manager())) {
                from_mandatory_modifiers = *modifiers;
              } else {
                is_target = false;
              }
            }

            if (is_target) {
              if (!condition_manager_.is_fulfilled(front_input_event,
                                                   output_event_queue->get_manipulator_environment())) {
                is_target = false;
              }
            }

            // ----------------------------------------

            if (is_target) {
              current_manipulated_original_event = std::make_shared<manipulated_original_event>(front_input_event.get_device_id(),
                                                                                                front_input_event.get_original_event(),
                                                                                                from_mandatory_modifiers,
                                                                                                front_input_event.get_time_stamp());
              manipulated_original_events_.push_back(current_manipulated_original_event);
            }
            break;
          }

          case event_type::key_up: {
            // event_type::key_up

            // Check original_event in order to determine the correspond key_down is manipulated.

            auto it = std::find_if(std::begin(manipulated_original_events_),
                                   std::end(manipulated_original_events_),
                                   [&](const auto& manipulated_original_event) {
                                     return manipulated_original_event->get_device_id() == front_input_event.get_device_id() &&
                                            manipulated_original_event->get_original_event() == front_input_event.get_original_event();
                                   });
            if (it != std::end(manipulated_original_events_)) {
              current_manipulated_original_event = *it;
              manipulated_original_events_.erase(it);
            }
            break;
          }

          case event_type::single:
            break;
        }

        if (current_manipulated_original_event) {
          front_input_event.set_valid(false);

          uint64_t time_stamp_delay = 0;

          // Release from_mandatory_modifiers

          if (front_input_event.get_event_type() == event_type::key_down) {
            post_lazy_modifier_key_events(front_input_event,
                                          current_manipulated_original_event->get_from_mandatory_modifiers(),
                                          event_type::key_up,
                                          time_stamp_delay,
                                          *output_event_queue);
          }

          // Send events

          switch (front_input_event.get_event_type()) {
            case event_type::key_down:
              for (auto it = std::begin(to_); it != std::end(to_); std::advance(it, 1)) {
                if (auto event = it->to_event()) {
                  // to_modifier down, to_key down, to_key up, to_modifier up

                  auto to_modifier_events = it->make_modifier_events();

                  {
                    bool lazy = !preserve_to_modifiers_down() || it->get_lazy();
                    for (const auto& e : to_modifier_events) {
                      output_event_queue->emplace_back_event(front_input_event.get_device_id(),
                                                             front_input_event.get_time_stamp() + time_stamp_delay++,
                                                             e,
                                                             event_type::key_down,
                                                             front_input_event.get_original_event(),
                                                             lazy);
                    }
                  }

                  output_event_queue->emplace_back_event(front_input_event.get_device_id(),
                                                         front_input_event.get_time_stamp() + time_stamp_delay++,
                                                         *event,
                                                         event_type::key_down,
                                                         front_input_event.get_original_event(),
                                                         it->get_lazy());

                  if (it != std::end(to_) - 1 || !it->get_repeat()) {
                    output_event_queue->emplace_back_event(front_input_event.get_device_id(),
                                                           front_input_event.get_time_stamp() + time_stamp_delay++,
                                                           *event,
                                                           event_type::key_up,
                                                           front_input_event.get_original_event(),
                                                           it->get_lazy());
                  } else {
                    current_manipulated_original_event->get_events_at_key_up().emplace_back_event(*event,
                                                                                                  event_type::key_up,
                                                                                                  it->get_lazy());
                  }

                  {
                    bool lazy = !preserve_to_modifiers_down() || it->get_lazy();
                    for (const auto& e : to_modifier_events) {
                      if (it == std::end(to_) - 1 && preserve_to_modifiers_down()) {
                        current_manipulated_original_event->get_events_at_key_up().emplace_back_event(e,
                                                                                                      event_type::key_up,
                                                                                                      lazy);
                      } else {
                        output_event_queue->emplace_back_event(front_input_event.get_device_id(),
                                                               front_input_event.get_time_stamp() + time_stamp_delay++,
                                                               e,
                                                               event_type::key_up,
                                                               front_input_event.get_original_event(),
                                                               lazy);
                      }
                    }
                  }
                }
              }
              break;

            case event_type::key_up: {
              post_events_at_key_up(front_input_event,
                                    *current_manipulated_original_event,
                                    time_stamp_delay,
                                    *output_event_queue);

              post_extra_to_events(front_input_event,
                                   to_after_key_up_,
                                   time_stamp_delay,
                                   *output_event_queue);

              uint64_t nanoseconds = time_utility::absolute_to_nano(front_input_event.get_time_stamp() - current_manipulated_original_event->get_key_down_time_stamp());
              if (current_manipulated_original_event->get_alone() &&
                  nanoseconds < parameters_.get_basic_to_if_alone_timeout_milliseconds() * NSEC_PER_MSEC) {
                post_extra_to_events(front_input_event,
                                     to_if_alone_,
                                     time_stamp_delay,
                                     *output_event_queue);
              }
            } break;

            case event_type::single:
              break;
          }

          // Restore from_mandatory_modifiers

          if ((front_input_event.get_event_type() == event_type::key_down && !preserve_from_mandatory_modifiers_up()) ||
              (front_input_event.get_event_type() == event_type::key_up && preserve_from_mandatory_modifiers_up())) {
            post_lazy_modifier_key_events(front_input_event,
                                          current_manipulated_original_event->get_from_mandatory_modifiers(),
                                          event_type::key_down,
                                          time_stamp_delay,
                                          *output_event_queue);
          }

          // to_if_held_down_

          if (to_if_held_down_) {
            to_if_held_down_->setup(front_input_event,
                                    current_manipulated_original_event,
                                    output_event_queue,
                                    parameters_.get_basic_to_if_held_down_threshold_milliseconds());
          }

          // to_delayed_action_

          if (to_delayed_action_) {
            to_delayed_action_->setup(front_input_event,
                                      current_manipulated_original_event->get_from_mandatory_modifiers(),
                                      output_event_queue,
                                      parameters_.get_basic_to_delayed_action_delay_milliseconds());
          }

          // increase_time_stamp_delay

          if (time_stamp_delay > 0) {
            output_event_queue->increase_time_stamp_delay(time_stamp_delay - 1);
          }
        }
      }
    }
  }

  virtual bool active(void) const {
    return !manipulated_original_events_.empty();
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    for (const auto& events : {to_,
                               to_after_key_up_,
                               to_if_alone_}) {
      for (const auto& e : events) {
        if (e.needs_virtual_hid_pointing()) {
          return true;
        }
      }
    }

    if (to_if_held_down_) {
      if (to_if_held_down_->needs_virtual_hid_pointing()) {
        return true;
      }
    }

    if (to_delayed_action_) {
      if (to_delayed_action_->needs_virtual_hid_pointing()) {
        return true;
      }
    }

    return false;
  }

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::queued_event& front_input_event,
                                                                          event_queue& output_event_queue) {
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
    manipulated_original_events_.erase(std::remove_if(std::begin(manipulated_original_events_),
                                                      std::end(manipulated_original_events_),
                                                      [&](const auto& e) {
                                                        return e->get_device_id() == device_id;
                                                      }),
                                       std::end(manipulated_original_events_));
  }

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());

    if (to_if_held_down_) {
      to_if_held_down_->cancel(front_input_event);
    }

    if (to_delayed_action_) {
      to_delayed_action_->cancel(front_input_event);
    }
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  virtual void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
    if (to_if_held_down_) {
      to_if_held_down_->manipulator_timer_invoked(timer_id);
    }

    if (to_delayed_action_) {
      to_delayed_action_->manipulator_timer_invoked(timer_id);
    }
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void post_events_at_key_up(const event_queue::queued_event& front_input_event,
                             manipulated_original_event& current_manipulated_original_event,
                             uint64_t& time_stamp_delay,
                             event_queue& output_event_queue) const {
    for (const auto& e : current_manipulated_original_event.get_events_at_key_up().get_events()) {
      output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                            front_input_event.get_time_stamp() + time_stamp_delay++,
                                            e.get_event(),
                                            e.get_event_type(),
                                            front_input_event.get_original_event(),
                                            e.get_lazy());
    }
    current_manipulated_original_event.get_events_at_key_up().clear_events();
  }

private:
  bool preserve_from_mandatory_modifiers_up(void) const {
    if (to_.empty()) {
      return false;
    }

    if (auto event = to_.back().to_event()) {
      if (auto key_code = event->get_key_code()) {
        if (types::make_modifier_flag(*key_code) != boost::none) {
          return true;
        }
      }
    }

    return false;
  }

  bool preserve_to_modifiers_down(void) const {
    return preserve_from_mandatory_modifiers_up();
  }

  void post_lazy_modifier_key_events(const event_queue::queued_event& front_input_event,
                                     const std::unordered_set<modifier_flag>& modifiers,
                                     event_type event_type,
                                     uint64_t& time_stamp_delay,
                                     event_queue& output_event_queue) {
    for (const auto& m : modifiers) {
      if (auto key_code = types::make_key_code(m)) {
        event_queue::queued_event event(front_input_event.get_device_id(),
                                        front_input_event.get_time_stamp() + time_stamp_delay++,
                                        event_queue::queued_event::event(*key_code),
                                        event_type,
                                        front_input_event.get_original_event(),
                                        true);
        output_event_queue.push_back_event(event);
      }
    }
  }

  void post_extra_to_events(const event_queue::queued_event& front_input_event,
                            const std::vector<to_event_definition>& to_events,
                            uint64_t& time_stamp_delay,
                            event_queue& output_event_queue) {
    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      auto& to = *it;
      if (auto event = to.to_event()) {
        auto to_modifier_events = to.make_modifier_events();

        for (const auto& e : to_modifier_events) {
          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                front_input_event.get_time_stamp() + time_stamp_delay++,
                                                e,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                true);
        }

        output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              *event,
                                              event_type::key_down,
                                              front_input_event.get_original_event(),
                                              it->get_lazy());

        output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              it->get_lazy());

        for (const auto& e : to_modifier_events) {
          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                front_input_event.get_time_stamp() + time_stamp_delay++,
                                                e,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                true);
        }
      }
    }
  }

  void unset_alone_if_needed(const event_queue::queued_event::event& event,
                             event_type event_type) {
    if (event.get_type() == event_queue::queued_event::event::type::key_code ||
        event.get_type() == event_queue::queued_event::event::type::consumer_key_code ||
        event.get_type() == event_queue::queued_event::event::type::pointing_button) {
      if (event_type == event_type::key_down) {
        goto run;
      }
    }
    if (auto pointing_motion = event.get_pointing_motion()) {
      if (pointing_motion->get_vertical_wheel() != 0 ||
          pointing_motion->get_horizontal_wheel() != 0) {
        goto run;
      }
    }

    return;

  run:
    for (auto& e : manipulated_original_events_) {
      e->unset_alone();
    }
  }

  core_configuration::profile::complex_modifications::parameters parameters_;

  from_event_definition from_;
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_after_key_up_;
  std::vector<to_event_definition> to_if_alone_;
  std::unique_ptr<to_if_held_down> to_if_held_down_;
  std::unique_ptr<to_delayed_action> to_delayed_action_;

  std::vector<std::shared_ptr<manipulated_original_event>> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
