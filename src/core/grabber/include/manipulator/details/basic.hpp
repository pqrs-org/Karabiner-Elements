#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
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
                               const std::unordered_set<modifier_flag> from_mandatory_modifiers) : device_id_(device_id),
                                                                                                   original_event_(original_event),
                                                                                                   from_mandatory_modifiers_(from_mandatory_modifiers) {
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

    bool operator==(const manipulated_original_event& other) const {
      // Do not compare `from_mandatory_modifiers_`.
      return get_device_id() == other.get_device_id() &&
             get_original_event() == other.get_original_event();
    }

  private:
    device_id device_id_;
    event_queue::queued_event::event original_event_;
    std::unordered_set<modifier_flag> from_mandatory_modifiers_;
  };

  basic(const nlohmann::json& json) : base(),
                                      from_(json.find("from") != std::end(json) ? json["from"] : nlohmann::json()) {
    {
      const std::string key = "to";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          to_.emplace_back(j);
        }
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
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
    if (!front_input_event.get_valid()) {
      return;
    }

    bool is_target = false;

    if (auto key_code = front_input_event.get_event().get_key_code()) {
      if (from_.get_key_code() == key_code) {
        is_target = true;
      }
    }
    if (auto pointing_button = front_input_event.get_event().get_pointing_button()) {
      if (from_.get_pointing_button() == pointing_button) {
        is_target = true;
      }
    }

    if (is_target) {
      std::unordered_set<modifier_flag> from_mandatory_modifiers;

      if (front_input_event.get_event_type() == event_type::key_down) {

        if (!valid_) {
          is_target = false;
        }

        if (auto modifiers = from_.test_modifiers(output_event_queue.get_modifier_flag_manager())) {
          from_mandatory_modifiers = *modifiers;
        } else {
          is_target = false;
        }

        if (is_target) {
          manipulated_original_events_.emplace_back(front_input_event.get_device_id(),
                                                    front_input_event.get_original_event(),
                                                    from_mandatory_modifiers);
        }

      } else {
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
          manipulated_original_events_.erase(it);
        } else {
          is_target = false;
        }
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

        for (size_t i = 0; i < to_.size(); ++i) {
          if (auto event = to_[i].to_event()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              enqueue_to_modifiers(to_[i],
                                   event_type::key_down,
                                   front_input_event,
                                   time_stamp_delay,
                                   output_event_queue);

              output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                    front_input_event.get_time_stamp() + time_stamp_delay++,
                                                    *event,
                                                    event_type::key_down,
                                                    front_input_event.get_original_event());

              if (i != to_.size() - 1) {
                output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                      front_input_event.get_time_stamp() + time_stamp_delay++,
                                                      *event,
                                                      event_type::key_up,
                                                      front_input_event.get_original_event());
              }

              if (i != to_.size() - 1 || !preserve_to_modifiers_down()) {
                enqueue_to_modifiers(to_[i],
                                     event_type::key_up,
                                     front_input_event,
                                     time_stamp_delay,
                                     output_event_queue);
              }

            } else {
              // event_type::key_up

              if (i == to_.size() - 1) {
                output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                      front_input_event.get_time_stamp() + time_stamp_delay++,
                                                      *event,
                                                      event_type::key_up,
                                                      front_input_event.get_original_event());

                if (preserve_to_modifiers_down()) {
                  enqueue_to_modifiers(to_[i],
                                       event_type::key_up,
                                       front_input_event,
                                       time_stamp_delay,
                                       output_event_queue);
                }
              }
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

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void enqueue_to_modifiers(const to_event_definition& to,
                            event_type event_type,
                            const event_queue::queued_event& front_input_event,
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
                                                !preserve_to_modifiers_down());
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

  from_event_definition from_;
  std::vector<to_event_definition> to_;

  std::vector<manipulated_original_event> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
