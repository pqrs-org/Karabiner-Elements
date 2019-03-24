#pragma once

#include "../../types.hpp"
#include "../base.hpp"
#include "core_configuration/core_configuration.hpp"
#include "event_sender.hpp"
#include "from_event_definition.hpp"
#include "krbn_notification_center.hpp"
#include "manipulated_original_event/manipulated_original_event.hpp"
#include "to_delayed_action.hpp"
#include "to_if_held_down.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/dispatcher.hpp>
#include <unordered_set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
class basic final : public base, public pqrs::dispatcher::extra::dispatcher_client {
public:
  basic(const nlohmann::json& json,
        const core_configuration::details::complex_modifications_parameters& parameters) : base(),
                                                                                           dispatcher_client(),
                                                                                           parameters_(parameters) {
    try {
      if (!json.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
      }

      for (const auto& [key, value] : json.items()) {
        if (key == "from") {
          try {
            from_ = value.get<from_event_definition>();
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
          }

        } else if (key == "to") {
          if (value.is_object()) {
            try {
              to_ = std::vector<to_event_definition>{
                  value.get<to_event_definition>(),
              };
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
            }

          } else if (value.is_array()) {
            try {
              to_ = value.get<std::vector<to_event_definition>>();
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, value.dump()));
          }

        } else if (key == "to_after_key_up") {
          if (value.is_object()) {
            try {
              to_after_key_up_ = std::vector<to_event_definition>{
                  value.get<to_event_definition>(),
              };
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
            }

          } else if (value.is_array()) {
            try {
              to_after_key_up_ = value.get<std::vector<to_event_definition>>();
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, value.dump()));
          }

        } else if (key == "to_if_alone") {
          if (value.is_object()) {
            try {
              to_if_alone_ = std::vector<to_event_definition>{
                  value.get<to_event_definition>(),
              };
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
            }

          } else if (value.is_array()) {
            try {
              to_if_alone_ = value.get<std::vector<to_event_definition>>();
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }

          } else {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, value.dump()));
          }

        } else if (key == "to_if_held_down") {
          try {
            to_if_held_down_ = std::make_shared<to_if_held_down>(value);
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
          }

        } else if (key == "to_delayed_action") {
          try {
            to_delayed_action_ = std::make_shared<to_delayed_action>(value);
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
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

    } catch (...) {
      detach_from_dispatcher();
      throw;
    }
  }

  basic(const from_event_definition& from,
        const to_event_definition& to) : base(),
                                         dispatcher_client(),
                                         from_(from),
                                         to_({to}) {
  }

  virtual ~basic(void) {
    detach_from_dispatcher();
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) {
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

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       const std::shared_ptr<event_queue::queue>& output_event_queue,
                                       absolute_time_point now) {
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

      if (!front_input_event.get_valid()) {
        return manipulate_result::passed;
      }

      // ----------------------------------------

      bool is_target = from_event_definition::test_event(front_input_event.get_event(), from_);

      if (is_target) {
        std::shared_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event;

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
                if (auto modifiers = from_.get_from_modifiers_definition().test_modifiers(output_event_queue->get_modifier_flag_manager())) {
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

              std::unordered_set<manipulated_original_event::from_event> from_events;

              {
                std::vector<event_queue::event> ordered_key_down_events;
                std::vector<event_queue::event> ordered_key_up_events;
                std::chrono::milliseconds simultaneous_threshold_milliseconds(parameters_.get_basic_simultaneous_threshold_milliseconds());
                auto end_time_stamp = front_input_event.get_event_time_stamp().get_time_stamp() +
                                      pqrs::osx::chrono::make_absolute_time_duration(simultaneous_threshold_milliseconds);

                for (const auto& entry : input_event_queue.get_entries()) {
                  if (!is_target) {
                    break;
                  }

                  if (!entry.get_valid()) {
                    continue;
                  }

                  if (end_time_stamp < entry.get_event_time_stamp().get_time_stamp()) {
                    break;
                  }

                  manipulated_original_event::from_event fe(entry.get_device_id(),
                                                            entry.get_event(),
                                                            entry.get_original_event());

                  switch (entry.get_event_type()) {
                    case event_type::key_down:
                      if (from_event_definition::test_event(entry.get_event(), from_)) {
                        // Insert the first event if the same events are arrived from different device.

                        if (std::none_of(std::begin(from_events),
                                         std::end(from_events),
                                         [&](auto& e) {
                                           return fe.get_event() == e.get_event();
                                         })) {
                          from_events.insert(fe);
                          ordered_key_down_events.push_back(entry.get_event());
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
                                             return e == entry.get_event();
                                           })) {
                            ordered_key_up_events.push_back(entry.get_event());
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
                    case simultaneous_options::key_order::insensitive:
                      // Do nothing
                      break;

                    case simultaneous_options::key_order::strict:
                    case simultaneous_options::key_order::strict_inverse:
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
                // Update input_delay_duration

                if (is_target) {
                  auto found = all_from_events_found(from_events);
                  if (needs_wait_key_up || !found) {
                    auto d = std::max(front_input_event.get_event_time_stamp().get_input_delay_duration(),
                                      pqrs::osx::chrono::make_absolute_time_duration(simultaneous_threshold_milliseconds));
                    front_input_event.get_event_time_stamp().set_input_delay_duration(d);

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

                      output_event_queue->emplace_back_entry(front_input_event.get_device_id(),
                                                             front_input_event.get_event_time_stamp(),
                                                             event_queue::event::make_stop_keyboard_repeat_event(),
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
                  current_manipulated_original_event =
                      std::make_shared<manipulated_original_event::manipulated_original_event>(
                          from_events,
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

          absolute_time_duration time_stamp_delay(0);

          // Send events

          switch (front_input_event.get_event_type()) {
            case event_type::key_down:
              // Release from_mandatory_modifiers

              event_sender::post_from_mandatory_modifiers_key_up(front_input_event,
                                                                 *current_manipulated_original_event,
                                                                 time_stamp_delay,
                                                                 *output_event_queue);

              event_sender::post_events_at_key_down(front_input_event,
                                                    to_,
                                                    *current_manipulated_original_event,
                                                    time_stamp_delay,
                                                    *output_event_queue);

              if (!event_sender::is_last_to_event_modifier_key_event(to_)) {
                event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                                     *current_manipulated_original_event,
                                                                     time_stamp_delay,
                                                                     *output_event_queue);
              }

              break;

            case event_type::key_up: {
              bool skip = false;

              switch (from_.get_simultaneous_options().get_key_up_when()) {
                case simultaneous_options::key_up_when::any:
                  break;

                case simultaneous_options::key_up_when::all:
                  if (!current_manipulated_original_event->get_from_events().empty()) {
                    skip = true;
                  }
                  break;
              }

              if (!skip) {
                if (!current_manipulated_original_event->get_key_up_posted()) {
                  current_manipulated_original_event->set_key_up_posted(true);

                  // to_

                  event_sender::post_events_at_key_up(front_input_event,
                                                      *current_manipulated_original_event,
                                                      time_stamp_delay,
                                                      *output_event_queue);

                  event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                                       *current_manipulated_original_event,
                                                                       time_stamp_delay,
                                                                       *output_event_queue);

                  // to_if_alone_

                  if (!to_if_alone_.empty()) {
                    auto duration = pqrs::osx::chrono::make_milliseconds(front_input_event.get_event_time_stamp().get_time_stamp() - current_manipulated_original_event->get_key_down_time_stamp());
                    if (current_manipulated_original_event->get_alone() &&
                        duration < std::chrono::milliseconds(parameters_.get_basic_to_if_alone_timeout_milliseconds())) {
                      event_sender::post_from_mandatory_modifiers_key_up(front_input_event,
                                                                         *current_manipulated_original_event,
                                                                         time_stamp_delay,
                                                                         *output_event_queue);

                      event_sender::post_extra_to_events(front_input_event,
                                                         to_if_alone_,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);

                      event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                                           *current_manipulated_original_event,
                                                                           time_stamp_delay,
                                                                           *output_event_queue);
                    }
                  }

                  // to_after_key_up_

                  if (!to_after_key_up_.empty()) {
                    event_sender::post_from_mandatory_modifiers_key_up(front_input_event,
                                                                       *current_manipulated_original_event,
                                                                       time_stamp_delay,
                                                                       *output_event_queue);

                    event_sender::post_extra_to_events(front_input_event,
                                                       to_after_key_up_,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);

                    event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                                         *current_manipulated_original_event,
                                                                         time_stamp_delay,
                                                                         *output_event_queue);
                  }
                }

                // simultaneous_options.to_after_key_up_

                if (!from_.get_simultaneous_options().get_to_after_key_up().empty()) {
                  if (current_manipulated_original_event->get_from_events().empty()) {
                    event_sender::post_from_mandatory_modifiers_key_up(front_input_event,
                                                                       *current_manipulated_original_event,
                                                                       time_stamp_delay,
                                                                       *output_event_queue);

                    event_sender::post_extra_to_events(front_input_event,
                                                       from_.get_simultaneous_options().get_to_after_key_up(),
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);

                    event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
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
            to_if_held_down_->setup(front_input_event,
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

          if (time_stamp_delay > absolute_time_duration(0)) {
            output_event_queue->increase_time_stamp_delay(time_stamp_delay - absolute_time_duration(1));
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

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) {
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) {
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

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  const std::vector<to_event_definition>& get_to_after_key_up(void) const {
    return to_after_key_up_;
  }

  const std::vector<to_event_definition>& get_to_if_alone(void) const {
    return to_if_alone_;
  }

  std::shared_ptr<to_if_held_down> get_to_if_held_down(void) const {
    return to_if_held_down_;
  }

  std::shared_ptr<to_delayed_action> get_to_delayed_action(void) const {
    return to_delayed_action_;
  }

private:
  bool all_from_events_found(const std::unordered_set<manipulated_original_event::from_event>& from_events) const {
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

  void unset_alone_if_needed(const event_queue::event& event,
                             event_type event_type) {
    if (event.get_type() == event_queue::event::type::key_code ||
        event.get_type() == event_queue::event::type::consumer_key_code ||
        event.get_type() == event_queue::event::type::pointing_button) {
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

  core_configuration::details::complex_modifications_parameters parameters_;

  from_event_definition from_;
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_after_key_up_;
  std::vector<to_event_definition> to_if_alone_;
  std::shared_ptr<to_if_held_down> to_if_held_down_;
  std::shared_ptr<to_delayed_action> to_delayed_action_;

  std::vector<std::shared_ptr<manipulated_original_event::manipulated_original_event>> manipulated_original_events_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
