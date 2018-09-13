#pragma once

#include "core_configuration.hpp"
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
#include "basic/from_event_definition.hpp"
#include "basic/manipulated_original_event.hpp"

#include "basic/to_delayed_action.hpp"
#include "basic/to_if_held_down.hpp"

  basic(const nlohmann::json& json,
        const core_configuration::profile::complex_modifications::parameters& parameters,
        std::weak_ptr<manipulator_dispatcher> weak_manipulator_dispatcher,
        std::weak_ptr<manipulator_timer> weak_manipulator_timer) : base(),
                                                                   parameters_(parameters),
                                                                   manipulator_object_id_(make_new_manipulator_object_id()),
                                                                   weak_manipulator_dispatcher_(weak_manipulator_dispatcher),
                                                                   weak_manipulator_timer_(weak_manipulator_timer),
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

    initialize();
  }

  basic(const from_event_definition& from,
        const to_event_definition& to,
        std::weak_ptr<manipulator_dispatcher> weak_manipulator_dispatcher,
        std::weak_ptr<manipulator_timer> weak_manipulator_timer) : base(),
                                                                   manipulator_object_id_(make_new_manipulator_object_id()),
                                                                   weak_manipulator_dispatcher_(weak_manipulator_dispatcher),
                                                                   weak_manipulator_timer_(weak_manipulator_timer),
                                                                   from_(from),
                                                                   to_({to}) {
    initialize();
  }

  virtual ~basic(void) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->enqueue(
          manipulator_object_id_,
          [this] {
            to_if_held_down_ = nullptr;
            to_delayed_action_ = nullptr;
          });
    }

    // detach

    if (auto manipulator_timer = weak_manipulator_timer_.lock()) {
      manipulator_timer->detach(manipulator_object_id_);
    }

    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->detach(manipulator_object_id_);
    }
  }

  virtual bool already_manipulated(const event_queue::queued_event& front_input_event) {
    // Skip if the key_down event is already manipulated by `simultaneous`.

    manipulated_original_event::from_event from_event(front_input_event.get_device_id(),
                                                      front_input_event.get_event(),
                                                      front_input_event.get_original_event());

    switch (front_input_event.get_event_type()) {
      case event_type::key_down:
        for (const auto& e : manipulated_original_events_) {
          if (e->from_event_exists(from_event)) {
            return true;
          }
        }
        break;

      case event_type::key_up:
      case event_type::single:
        // Do nothing
        break;
    }

    return false;
  }

  virtual manipulate_result manipulate(event_queue::queued_event& front_input_event,
                                       const event_queue& input_event_queue,
                                       const std::shared_ptr<event_queue>& output_event_queue,
                                       absolute_time now) {
    if (output_event_queue) {
      unset_alone_if_needed(front_input_event.get_event(),
                            front_input_event.get_event_type());

      if (to_if_held_down_) {
        to_if_held_down_->async_cancel(front_input_event);
      }

      if (to_delayed_action_) {
        to_delayed_action_->cancel(front_input_event);
      }

      // ----------------------------------------

      if (!front_input_event.get_valid()) {
        return manipulate_result::passed;
      }

      // ----------------------------------------

      bool is_target = from_event_definition::test_event(front_input_event.get_event(), from_);

      if (is_target) {
        std::shared_ptr<manipulated_original_event> current_manipulated_original_event;

        manipulated_original_event::from_event from_event(front_input_event.get_device_id(),
                                                          front_input_event.get_event(),
                                                          front_input_event.get_original_event());

        switch (front_input_event.get_event_type()) {
          case event_type::key_down: {
            std::unordered_set<modifier_flag> from_mandatory_modifiers;

            // ----------------------------------------
            // Check whether event is target.

            if (!valid_) {
              is_target = false;
            }

            if (is_target) {
              // Check mandatory_modifiers and conditions

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

              // Check all from_events_ are pressed

              std::unordered_set<manipulated_original_event::from_event, manipulated_original_event::from_event_hash> from_events;

              {
                std::vector<event_queue::queued_event::event> ordered_key_down_events;
                std::vector<event_queue::queued_event::event> ordered_key_up_events;
                std::chrono::milliseconds simultaneous_threshold_milliseconds(parameters_.get_basic_simultaneous_threshold_milliseconds());
                auto end_time_stamp = front_input_event.get_event_time_stamp().get_time_stamp() +
                                      time_utility::to_absolute_time(simultaneous_threshold_milliseconds);

                for (const auto& queued_event : input_event_queue.get_events()) {
                  if (!is_target) {
                    break;
                  }

                  if (!queued_event.get_valid()) {
                    continue;
                  }

                  if (end_time_stamp < queued_event.get_event_time_stamp().get_time_stamp()) {
                    break;
                  }

                  manipulated_original_event::from_event fe(queued_event.get_device_id(),
                                                            queued_event.get_event(),
                                                            queued_event.get_original_event());

                  switch (queued_event.get_event_type()) {
                    case event_type::key_down:
                      if (from_event_definition::test_event(queued_event.get_event(), from_)) {
                        // Insert the first event if the same events are arrived from different device.

                        if (std::none_of(std::begin(from_events),
                                         std::end(from_events),
                                         [&](auto& e) {
                                           return fe.get_event() == e.get_event();
                                         })) {
                          from_events.insert(fe);
                          ordered_key_down_events.push_back(queued_event.get_event());
                        }

                      } else {
                        // Do not manipulate if another event arrived.

                        if (!from_.get_simultaneous_options().get_detect_key_down_uninterruptedly()) {
                          if (!all_from_events_found(from_events)) {
                            is_target = false;
                          }
                        }
                      }

                      break;

                    case event_type::key_up:
                      // Do not manipulate if pressed key is released before all from events are pressed.

                      if (from_events.find(fe) != std::end(from_events)) {
                        if (!all_from_events_found(from_events)) {
                          is_target = false;
                        }

                        if (is_target) {
                          if (std::none_of(std::begin(ordered_key_up_events),
                                           std::end(ordered_key_up_events),
                                           [&](auto& e) {
                                             return e == queued_event.get_event();
                                           })) {
                            ordered_key_up_events.push_back(queued_event.get_event());
                          }
                        }
                      }

                      break;

                    case event_type::single:
                      // Do nothing
                      break;
                  }
                }

                // from_events will be empty if all input events's time_stamp > end_time_stamp.

                if (is_target) {
                  if (from_events.empty()) {
                    is_target = false;
                  }
                }

                // Test key_order

                if (is_target) {
                  if (!from_event_definition::test_key_order(ordered_key_down_events,
                                                             from_.get_simultaneous_options().get_key_down_order(),
                                                             from_.get_event_definitions())) {
                    is_target = false;
                  }
                }

                bool needs_wait_key_up = false;

                if (is_target) {
                  switch (from_.get_simultaneous_options().get_key_up_order()) {
                    case from_event_definition::simultaneous_options::key_order::insensitive:
                      // Do nothing
                      break;

                    case from_event_definition::simultaneous_options::key_order::strict:
                    case from_event_definition::simultaneous_options::key_order::strict_inverse:
                      if (!from_event_definition::test_key_order(ordered_key_up_events,
                                                                 from_.get_simultaneous_options().get_key_up_order(),
                                                                 from_.get_event_definitions())) {
                        is_target = false;
                      } else {
                        if (ordered_key_up_events.size() < from_.get_event_definitions().size() - 1) {
                          needs_wait_key_up = true;
                        }
                      }
                      break;
                  }
                }

                // Wait if the front_input_event is `simultaneous` target and `simultaneous` is not canceled.
                // Update input_delay_time_stamp

                if (is_target) {
                  auto found = all_from_events_found(from_events);
                  if (needs_wait_key_up || !found) {
                    auto t = std::max(front_input_event.get_event_time_stamp().get_input_delay_time_stamp(),
                                      time_utility::to_absolute_time(simultaneous_threshold_milliseconds));
                    front_input_event.get_event_time_stamp().set_input_delay_time_stamp(t);

                    if (now < front_input_event.get_event_time_stamp().make_time_stamp_with_input_delay()) {

                      // We need to stop keyboard repeat when needs_wait_until_time_stamp.
                      //
                      // Example:
                      //   With "Change s+j to down_arrow".
                      //
                      //     (1) o key_down
                      //     (2) s key_down
                      //     (3) o key_up
                      //
                      //  The (2) returns needs_wait_until_time_stamp and stop until simultaneous_threshold_milliseconds.
                      //  If simultaneous_threshold_milliseconds is greater than keyboard repeat delay,
                      //  the o key is repeated even if the s key is pressed.
                      //  To avoid this issue, we post stop_keyboard_repeat here.

                      output_event_queue->emplace_back_event(front_input_event.get_device_id(),
                                                             front_input_event.get_event_time_stamp(),
                                                             event_queue::queued_event::event::make_stop_keyboard_repeat_event(),
                                                             event_type::single,
                                                             front_input_event.get_original_event(),
                                                             true);

                      return manipulate_result::needs_wait_until_time_stamp;

                    } else {
                      if (!found) {
                        is_target = false;
                      }
                    }
                  }
                }
              }

              // ----------------------------------------

              if (is_target) {
                // Add manipulated_original_event if not manipulated.

                if (!current_manipulated_original_event) {
                  current_manipulated_original_event = std::make_shared<manipulated_original_event>(from_events,
                                                                                                    from_mandatory_modifiers,
                                                                                                    front_input_event.get_event_time_stamp().get_time_stamp());
                  manipulated_original_events_.push_back(current_manipulated_original_event);
                }
              }
            }
            break;
          }

          case event_type::key_up: {
            // event_type::key_up

            // Check original_event in order to determine the correspond key_down is manipulated.

            auto it = std::find_if(std::begin(manipulated_original_events_),
                                   std::end(manipulated_original_events_),
                                   [&](const auto& manipulated_original_event) {
                                     return manipulated_original_event->from_event_exists(from_event);
                                   });
            if (it != std::end(manipulated_original_events_)) {
              current_manipulated_original_event = *it;
              current_manipulated_original_event->erase_from_event(from_event);
              if (current_manipulated_original_event->get_from_events().empty()) {
                manipulated_original_events_.erase(it);
              }
            }
            break;
          }

          case event_type::single:
            break;
        }

        if (current_manipulated_original_event) {
          front_input_event.set_valid(false);

          absolute_time time_stamp_delay(0);

          // Send events

          switch (front_input_event.get_event_type()) {
            case event_type::key_down:
              // Release from_mandatory_modifiers

              post_from_mandatory_modifiers_key_up(front_input_event,
                                                   *current_manipulated_original_event,
                                                   time_stamp_delay,
                                                   *output_event_queue);

              post_events_at_key_down(front_input_event,
                                      to_,
                                      *current_manipulated_original_event,
                                      time_stamp_delay,
                                      *output_event_queue);

              if (!is_last_to_event_modifier_key_event(to_)) {
                post_from_mandatory_modifiers_key_down(front_input_event,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);
              }

              break;

            case event_type::key_up: {
              bool skip = false;

              switch (from_.get_simultaneous_options().get_key_up_when()) {
                case from_event_definition::simultaneous_options::key_up_when::any:
                  break;

                case from_event_definition::simultaneous_options::key_up_when::all:
                  if (!current_manipulated_original_event->get_from_events().empty()) {
                    skip = true;
                  }
                  break;
              }

              if (!skip) {
                if (!current_manipulated_original_event->get_key_up_posted()) {
                  current_manipulated_original_event->set_key_up_posted(true);

                  // to_

                  post_events_at_key_up(front_input_event,
                                        *current_manipulated_original_event,
                                        time_stamp_delay,
                                        *output_event_queue);

                  post_from_mandatory_modifiers_key_down(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);

                  // to_if_alone_

                  if (!to_if_alone_.empty()) {
                    auto duration = time_utility::to_milliseconds(front_input_event.get_event_time_stamp().get_time_stamp() - current_manipulated_original_event->get_key_down_time_stamp());
                    if (current_manipulated_original_event->get_alone() &&
                        duration < std::chrono::milliseconds(parameters_.get_basic_to_if_alone_timeout_milliseconds())) {
                      post_from_mandatory_modifiers_key_up(front_input_event,
                                                           *current_manipulated_original_event,
                                                           time_stamp_delay,
                                                           *output_event_queue);

                      post_extra_to_events(front_input_event,
                                           to_if_alone_,
                                           *current_manipulated_original_event,
                                           time_stamp_delay,
                                           *output_event_queue);

                      post_from_mandatory_modifiers_key_down(front_input_event,
                                                             *current_manipulated_original_event,
                                                             time_stamp_delay,
                                                             *output_event_queue);
                    }
                  }

                  // to_after_key_up_

                  if (!to_after_key_up_.empty()) {
                    post_from_mandatory_modifiers_key_up(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);

                    post_extra_to_events(front_input_event,
                                         to_after_key_up_,
                                         *current_manipulated_original_event,
                                         time_stamp_delay,
                                         *output_event_queue);

                    post_from_mandatory_modifiers_key_down(front_input_event,
                                                           *current_manipulated_original_event,
                                                           time_stamp_delay,
                                                           *output_event_queue);
                  }
                }

                // simultaneous_options.to_after_key_up_

                if (!from_.get_simultaneous_options().get_to_after_key_up().empty()) {
                  if (current_manipulated_original_event->get_from_events().empty()) {
                    post_from_mandatory_modifiers_key_up(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);

                    post_extra_to_events(front_input_event,
                                         from_.get_simultaneous_options().get_to_after_key_up(),
                                         *current_manipulated_original_event,
                                         time_stamp_delay,
                                         *output_event_queue);

                    post_from_mandatory_modifiers_key_down(front_input_event,
                                                           *current_manipulated_original_event,
                                                           time_stamp_delay,
                                                           *output_event_queue);
                  }
                }
              }

              break;
            }

            case event_type::single:
              break;
          }

          // to_if_held_down_

          if (to_if_held_down_) {
            to_if_held_down_->async_setup(front_input_event,
                                          current_manipulated_original_event,
                                          output_event_queue,
                                          std::chrono::milliseconds(parameters_.get_basic_to_if_held_down_threshold_milliseconds()));
          }

          // to_delayed_action_

          if (to_delayed_action_) {
            to_delayed_action_->setup(front_input_event,
                                      current_manipulated_original_event,
                                      output_event_queue,
                                      std::chrono::milliseconds(parameters_.get_basic_to_delayed_action_delay_milliseconds()));
          }

          // increase_time_stamp_delay

          if (time_stamp_delay > absolute_time(0)) {
            output_event_queue->increase_time_stamp_delay(time_stamp_delay - absolute_time(1));
          }

          return manipulate_result::manipulated;
        }
      }
    }

    return manipulate_result::passed;
  }

  virtual bool active(void) const {
    return !manipulated_original_events_.empty();
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    for (const auto& events : {to_,
                               to_after_key_up_,
                               to_if_alone_}) {
      if (std::any_of(std::begin(events),
                      std::end(events),
                      [](auto& e) {
                        return e.needs_virtual_hid_pointing();
                      })) {
        return true;
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
                                             absolute_time time_stamp) {
    for (auto&& e : manipulated_original_events_) {
      e->erase_from_events_by_device_id(device_id);
    }

    manipulated_original_events_.erase(std::remove_if(std::begin(manipulated_original_events_),
                                                      std::end(manipulated_original_events_),
                                                      [&](const auto& e) {
                                                        return e->get_from_events().empty();
                                                      }),
                                       std::end(manipulated_original_events_));
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  std::weak_ptr<manipulator_dispatcher> get_weak_manipulator_dispatcher(void) const {
    return weak_manipulator_dispatcher_;
  }

  std::weak_ptr<manipulator_timer> get_weak_manipulator_timer(void) const {
    return weak_manipulator_timer_;
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void post_from_mandatory_modifiers_key_up(const event_queue::queued_event& front_input_event,
                                            manipulated_original_event& current_manipulated_original_event,
                                            absolute_time& time_stamp_delay,
                                            event_queue& output_event_queue) const {
    // ----------------------------------------
    // Make target modifiers

    std::unordered_set<modifier_flag> modifiers;

    for (const auto& m : current_manipulated_original_event.get_from_mandatory_modifiers()) {
      auto& key_up_posted_from_mandatory_modifiers = current_manipulated_original_event.get_key_up_posted_from_mandatory_modifiers();

      if (key_up_posted_from_mandatory_modifiers.find(m) != std::end(key_up_posted_from_mandatory_modifiers)) {
        continue;
      }

      // All from_mandatory_modifiers are usually pressed when `post_from_mandatory_modifiers_key_up` is called.
      // However, there are some exceptional cases.
      //
      // Example:
      //   - from:            left_shift+f2
      //   - to:              left_shift+f3
      //   - to_after_key_up: left_shift+tab
      //
      //   1. left_shift key_down
      //   2. f2 key_down
      //   3. left_shift key_up
      //   4. f2 key_up
      //
      //   to_after_key_up is called at 4.
      //   to_after_key_up calls `post_from_mandatory_modifiers_key_up` but `left_shift` is not pressed.
      //
      // We should not post key_up event in this case.

      if (!output_event_queue.get_modifier_flag_manager().is_pressed(m)) {
        continue;
      }

      modifiers.insert(m);
      key_up_posted_from_mandatory_modifiers.insert(m);
    }

    // ----------------------------------------

    post_lazy_modifier_key_events(front_input_event,
                                  modifiers,
                                  event_type::key_up,
                                  time_stamp_delay,
                                  output_event_queue);
  }

  void post_from_mandatory_modifiers_key_down(const event_queue::queued_event& front_input_event,
                                              manipulated_original_event& current_manipulated_original_event,
                                              absolute_time& time_stamp_delay,
                                              event_queue& output_event_queue) const {
    // ----------------------------------------
    // Make target modifiers

    std::unordered_set<modifier_flag> modifiers;

    for (const auto& m : current_manipulated_original_event.get_from_mandatory_modifiers()) {
      auto& key_up_posted_from_mandatory_modifiers = current_manipulated_original_event.get_key_up_posted_from_mandatory_modifiers();

      if (key_up_posted_from_mandatory_modifiers.find(m) == std::end(key_up_posted_from_mandatory_modifiers)) {
        continue;
      }

      modifiers.insert(m);
      key_up_posted_from_mandatory_modifiers.erase(m);
    }

    // ----------------------------------------

    post_lazy_modifier_key_events(front_input_event,
                                  modifiers,
                                  event_type::key_down,
                                  time_stamp_delay,
                                  output_event_queue);
  }

  void post_events_at_key_down(const event_queue::queued_event& front_input_event,
                               std::vector<to_event_definition> to_events,
                               manipulated_original_event& current_manipulated_original_event,
                               absolute_time& time_stamp_delay,
                               event_queue& output_event_queue) const {
    if (current_manipulated_original_event.get_halted()) {
      return;
    }

    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      if (auto event = it->get_event_definition().to_event()) {
        // to_modifier down, to_key down, to_key up, to_modifier up

        auto to_modifier_events = it->make_modifier_events();

        bool is_modifier_key_event = false;
        if (auto key_code = event->get_key_code()) {
          if (types::make_modifier_flag(*key_code) != boost::none) {
            is_modifier_key_event = true;
          }
        }

        {
          // Unset lazy if event is modifier key event in order to keep modifier keys order.

          bool lazy = !is_modifier_key_event || it->get_lazy();
          for (const auto& e : to_modifier_events) {
            auto t = front_input_event.get_event_time_stamp();
            t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

            output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                  t,
                                                  e,
                                                  event_type::key_down,
                                                  front_input_event.get_original_event(),
                                                  lazy);
          }
        }

        // Post key_down event

        {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());

          if (it->get_halt()) {
            current_manipulated_original_event.set_halted();
          }
        }

        // Post key_up event

        if (it != std::end(to_events) - 1 || !it->get_repeat()) {
          time_stamp_delay += time_utility::to_absolute_time(it->get_hold_down_milliseconds());

          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        } else {
          current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                       *event,
                                                                                       event_type::key_up,
                                                                                       front_input_event.get_original_event(),
                                                                                       it->get_lazy());
        }

        {
          for (const auto& e : to_modifier_events) {
            if (it == std::end(to_events) - 1 && is_modifier_key_event) {
              current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                           e,
                                                                                           event_type::key_up,
                                                                                           front_input_event.get_original_event(),
                                                                                           true);
            } else {
              auto t = front_input_event.get_event_time_stamp();
              t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

              output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                    t,
                                                    e,
                                                    event_type::key_up,
                                                    front_input_event.get_original_event(),
                                                    true);
            }
          }
        }
      }
    }
  }

  void post_events_at_key_up(const event_queue::queued_event& front_input_event,
                             manipulated_original_event& current_manipulated_original_event,
                             absolute_time& time_stamp_delay,
                             event_queue& output_event_queue) const {
    for (const auto& e : current_manipulated_original_event.get_events_at_key_up().get_events()) {
      auto t = front_input_event.get_event_time_stamp();
      t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);
      output_event_queue.push_back_event(e.make_queued_event(t));
    }
    current_manipulated_original_event.get_events_at_key_up().clear_events();
  }

private:
  void initialize(void) {
    if (auto manipulator_dispatcher = weak_manipulator_dispatcher_.lock()) {
      manipulator_dispatcher->async_attach(manipulator_object_id_);
    }

    if (auto manipulator_timer = weak_manipulator_timer_.lock()) {
      manipulator_timer->async_attach(manipulator_object_id_);
    }
  }

  bool all_from_events_found(const std::unordered_set<manipulated_original_event::from_event, manipulated_original_event::from_event_hash>& from_events) const {
    for (const auto& d : from_.get_event_definitions()) {
      if (std::none_of(std::begin(from_events),
                       std::end(from_events),
                       [&](auto& e) {
                         return from_event_definition::test_event(e.get_event(), d);
                       })) {
        return false;
      }
    }

    return true;
  }

  bool is_last_to_event_modifier_key_event(const std::vector<to_event_definition>& to_events) const {
    if (to_events.empty()) {
      return false;
    }

    if (auto event = to_events.back().get_event_definition().to_event()) {
      if (auto key_code = event->get_key_code()) {
        if (types::make_modifier_flag(*key_code) != boost::none) {
          return true;
        }
      }
    }

    return false;
  }

  void post_lazy_modifier_key_events(const event_queue::queued_event& front_input_event,
                                     const std::unordered_set<modifier_flag>& modifiers,
                                     event_type event_type,
                                     absolute_time& time_stamp_delay,
                                     event_queue& output_event_queue) const {
    for (const auto& m : modifiers) {
      if (auto key_code = types::make_key_code(m)) {
        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        event_queue::queued_event event(front_input_event.get_device_id(),
                                        t,
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
                            manipulated_original_event& current_manipulated_original_event,
                            absolute_time& time_stamp_delay,
                            event_queue& output_event_queue) const {
    if (current_manipulated_original_event.get_halted()) {
      return;
    }

    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      auto& to = *it;
      if (auto event = to.get_event_definition().to_event()) {
        auto to_modifier_events = to.make_modifier_events();

        // Post modifier events

        for (const auto& e : to_modifier_events) {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                e,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                true);
        }

        // Post key_down event

        {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());

          if (it->get_halt()) {
            current_manipulated_original_event.set_halted();
          }
        }

        // Post key_up event

        {
          time_stamp_delay += time_utility::to_absolute_time(it->get_hold_down_milliseconds());

          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        }

        // Post modifier events

        for (const auto& e : to_modifier_events) {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
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
  manipulator_object_id manipulator_object_id_;
  std::weak_ptr<manipulator_dispatcher> weak_manipulator_dispatcher_;
  std::weak_ptr<manipulator_timer> weak_manipulator_timer_;

  from_event_definition from_;
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_after_key_up_;
  std::vector<to_event_definition> to_if_alone_;
  std::unique_ptr<to_if_held_down> to_if_held_down_;
  std::unique_ptr<to_delayed_action> to_delayed_action_;

  std::vector<std::shared_ptr<manipulated_original_event>> manipulated_original_events_;
};

inline void from_json(const nlohmann::json& json, basic::from_event_definition::simultaneous_options::key_order& value) {
  auto s = json.get<std::string>();

  if (s == "insensitive") {
    value = basic::from_event_definition::simultaneous_options::key_order::insensitive;
  } else if (s == "strict") {
    value = basic::from_event_definition::simultaneous_options::key_order::strict;
  } else if (s == "strict_inverse") {
    value = basic::from_event_definition::simultaneous_options::key_order::strict_inverse;
  } else {
    logger::get_logger().error("complex_modifications json error: Unknown simultaneous_options::key_order: {0}", json.dump());
    value = basic::from_event_definition::simultaneous_options::key_order::insensitive;
  }
}

inline void from_json(const nlohmann::json& json, basic::from_event_definition::simultaneous_options::key_up_when& value) {
  auto s = json.get<std::string>();

  if (s == "any") {
    value = basic::from_event_definition::simultaneous_options::key_up_when::any;
  } else if (s == "all") {
    value = basic::from_event_definition::simultaneous_options::key_up_when::all;
  } else {
    logger::get_logger().error("complex_modifications json error: Unknown simultaneous_options::key_up_when: {0}", json.dump());
    value = basic::from_event_definition::simultaneous_options::key_up_when::any;
  }
}
} // namespace details
} // namespace manipulator
} // namespace krbn
