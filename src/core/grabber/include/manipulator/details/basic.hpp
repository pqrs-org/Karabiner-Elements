#pragma once

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
    manipulated_original_event(device_id device_id,
                               const event_queue::queued_event::event& original_event,
                               const std::unordered_set<modifier_flag> from_mandatory_modifiers,
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
  };

  basic(const nlohmann::json& json,
        const core_configuration::profile::complex_modifications::parameters& parameters) : base(),
                                                                                            parameters_(parameters),
                                                                                            from_(json.find("from") != std::end(json) ? json["from"] : nlohmann::json()) {
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
                          event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_event(),
                          front_input_event.get_event_type());

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
      std::unordered_set<modifier_flag> from_mandatory_modifiers;
      uint64_t key_down_time_stamp = 0;
      bool alone = false;

      switch (front_input_event.get_event_type()) {
        case event_type::key_down:
          // ----------------------------------------
          // Check whether event is target.

          if (!valid_) {
            is_target = false;
          }

          if (is_target) {
            if (auto modifiers = from_.test_modifiers(output_event_queue.get_modifier_flag_manager())) {
              from_mandatory_modifiers = *modifiers;
            } else {
              is_target = false;
            }
          }

          if (is_target) {
            if (!condition_manager_.is_fulfilled(front_input_event,
                                                 output_event_queue.get_manipulator_environment())) {
              is_target = false;
            }
          }

          // ----------------------------------------

          if (is_target) {
            manipulated_original_events_.emplace_back(front_input_event.get_device_id(),
                                                      front_input_event.get_original_event(),
                                                      from_mandatory_modifiers,
                                                      front_input_event.get_time_stamp());
          }
          break;

        case event_type::key_up: {
          // event_type::key_up

          // Check original_event in order to determine the correspond key_down is manipulated.

          auto it = std::find_if(std::begin(manipulated_original_events_),
                                 std::end(manipulated_original_events_),
                                 [&](const auto& manipulated_original_event) {
                                   return manipulated_original_event.get_device_id() == front_input_event.get_device_id() &&
                                          manipulated_original_event.get_original_event() == front_input_event.get_original_event();
                                 });
          if (it != std::end(manipulated_original_events_)) {
            from_mandatory_modifiers = it->get_from_mandatory_modifiers();
            key_down_time_stamp = it->get_key_down_time_stamp();
            alone = it->get_alone();
            manipulated_original_events_.erase(it);
          } else {
            is_target = false;
          }
          break;
        }

        case event_type::single:
          break;
      }

      if (is_target) {
        front_input_event.set_valid(false);

        uint64_t time_stamp_delay = 0;

        // Release from_mandatory_modifiers

        if (front_input_event.get_event_type() == event_type::key_down) {
          for (const auto& m : from_mandatory_modifiers) {
            if (auto key_code = types::get_key_code(m)) {
              event_queue::queued_event event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              event_queue::queued_event::event(*key_code),
                                              event_type::key_up,
                                              front_input_event.get_original_event(),
                                              true);
              output_event_queue.push_back_event(event);
            }
          }
        }

        // Send events

        for (auto it = std::begin(to_); it != std::end(to_); std::advance(it, 1)) {
          if (auto event = it->to_event()) {
            switch (front_input_event.get_event_type()) {
              case event_type::key_down:
                // to_modifier down, to_key down, to_key up, to_modifier up

                {
                  bool lazy = !preserve_to_modifiers_down();
                  enqueue_to_modifiers(*it,
                                       event_type::key_down,
                                       front_input_event,
                                       lazy,
                                       time_stamp_delay,
                                       output_event_queue);
                }

                output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                      front_input_event.get_time_stamp() + time_stamp_delay++,
                                                      *event,
                                                      event_type::key_down,
                                                      front_input_event.get_original_event());

                if (it != std::end(to_) - 1) {
                  output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                        front_input_event.get_time_stamp() + time_stamp_delay++,
                                                        *event,
                                                        event_type::key_up,
                                                        front_input_event.get_original_event());
                }

                if (it == std::end(to_) - 1 && preserve_to_modifiers_down()) {
                  // Do nothing
                } else {
                  bool lazy = !preserve_to_modifiers_down();
                  enqueue_to_modifiers(*it,
                                       event_type::key_up,
                                       front_input_event,
                                       lazy,
                                       time_stamp_delay,
                                       output_event_queue);
                }
                break;

              case event_type::key_up:
                if (it == std::end(to_) - 1) {
                  output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                        front_input_event.get_time_stamp() + time_stamp_delay++,
                                                        *event,
                                                        event_type::key_up,
                                                        front_input_event.get_original_event());

                  if (preserve_to_modifiers_down()) {
                    enqueue_to_modifiers(*it,
                                         event_type::key_up,
                                         front_input_event,
                                         false,
                                         time_stamp_delay,
                                         output_event_queue);
                  }

                  send_extra_to_events(front_input_event,
                                       to_after_key_up_,
                                       time_stamp_delay,
                                       output_event_queue);

                  uint64_t nanoseconds = time_utility::absolute_to_nano(front_input_event.get_time_stamp() - key_down_time_stamp);
                  if (alone &&
                      nanoseconds < parameters_.get_basic_to_if_alone_timeout_milliseconds() * NSEC_PER_MSEC) {
                    send_extra_to_events(front_input_event,
                                         to_if_alone_,
                                         time_stamp_delay,
                                         output_event_queue);
                  }
                }
                break;

              case event_type::single:
                break;
            }
          }
        }

        // Restore from_mandatory_modifiers

        if ((front_input_event.get_event_type() == event_type::key_down && !preserve_from_mandatory_modifiers_up()) ||
            (front_input_event.get_event_type() == event_type::key_up && preserve_from_mandatory_modifiers_up())) {
          for (const auto& m : from_mandatory_modifiers) {
            if (auto key_code = types::get_key_code(m)) {
              event_queue::queued_event event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              event_queue::queued_event::event(*key_code),
                                              event_type::key_down,
                                              front_input_event.get_original_event(),
                                              true);
              output_event_queue.push_back_event(event);
            }
          }
        }

        if (time_stamp_delay > 0) {
          output_event_queue.increase_time_stamp_delay(time_stamp_delay - 1);
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
        if (e.get_type() == event_definition::type::pointing_button) {
          return true;
        }
      }
    }

    return false;
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
    manipulated_original_events_.erase(std::remove_if(std::begin(manipulated_original_events_),
                                                      std::end(manipulated_original_events_),
                                                      [&](const auto& e) {
                                                        return e.get_device_id() == device_id;
                                                      }),
                                       std::end(manipulated_original_events_));
  }

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  virtual void force_post_modifier_key_event(const event_queue::queued_event& front_input_event,
                                             event_queue& output_event_queue) {
  }

  virtual void force_post_pointing_button_event(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void enqueue_to_modifiers(const to_event_definition& to,
                            event_type event_type,
                            const event_queue::queued_event& front_input_event,
                            bool lazy,
                            uint64_t& time_stamp_delay,
                            event_queue& output_event_queue) {
    for (const auto& modifier : to.get_modifiers()) {
      // `event_definition::get_modifiers` might return two modifier_flags.
      // (eg. `modifier_flag::left_shift` and `modifier_flag::right_shift` for `modifier::shift`.)
      // We use the first modifier_flag.

      auto modifier_flags = event_definition::get_modifier_flags(modifier);
      if (!modifier_flags.empty()) {
        auto modifier_flag = modifier_flags.front();
        if (auto key_code = types::get_key_code(modifier_flag)) {
          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                front_input_event.get_time_stamp() + time_stamp_delay++,
                                                event_queue::queued_event::event(*key_code),
                                                event_type,
                                                front_input_event.get_original_event(),
                                                lazy);
        }
      }
    }
  }

private:
  bool preserve_from_mandatory_modifiers_up(void) const {
    if (to_.empty()) {
      return false;
    }

    if (auto event = to_.back().to_event()) {
      if (auto key_code = event->get_key_code()) {
        if (types::get_modifier_flag(*key_code) != modifier_flag::zero) {
          return true;
        }
      }
    }

    return false;
  }

  bool preserve_to_modifiers_down(void) const {
    return preserve_from_mandatory_modifiers_up();
  }

  void send_extra_to_events(const event_queue::queued_event& front_input_event,
                            const std::vector<to_event_definition>& to_events,
                            uint64_t& time_stamp_delay,
                            event_queue& output_event_queue) {
    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      auto& to = *it;
      if (auto event = to.to_event()) {
        enqueue_to_modifiers(to,
                             event_type::key_down,
                             front_input_event,
                             true,
                             time_stamp_delay,
                             output_event_queue);

        output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              *event,
                                              event_type::key_down,
                                              front_input_event.get_original_event());

        output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                              front_input_event.get_time_stamp() + time_stamp_delay++,
                                              *event,
                                              event_type::key_up,
                                              front_input_event.get_original_event());

        enqueue_to_modifiers(to,
                             event_type::key_up,
                             front_input_event,
                             true,
                             time_stamp_delay,
                             output_event_queue);
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
    if (event.get_type() == event_queue::queued_event::event::type::pointing_vertical_wheel ||
        event.get_type() == event_queue::queued_event::event::type::pointing_horizontal_wheel) {
      if (auto integer_value = event.get_integer_value()) {
        if (*integer_value != 0) {
          goto run;
        }
      }
    }

    return;

  run:
    for (auto& e : manipulated_original_events_) {
      e.unset_alone();
    }
  }

  core_configuration::profile::complex_modifications::parameters parameters_;

  from_event_definition from_;
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_after_key_up_;
  std::vector<to_event_definition> to_if_alone_;

  std::vector<manipulated_original_event> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
