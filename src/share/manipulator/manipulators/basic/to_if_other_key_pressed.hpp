#pragma once

#include "../../types.hpp"
#include "event_sender.hpp"
#include "from_event_definition.hpp"
#include <algorithm>
#include <pqrs/json.hpp>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
class to_if_other_key_pressed final {
public:
  class entry final {
  public:
    explicit entry(const nlohmann::json& json) {
      pqrs::json::requires_object(json, "json");

      for (const auto& [key, value] : json.items()) {
        if (key == "other_keys") {
          pqrs::json::requires_array(value, "`" + key + "`");

          for (const auto& j : value) {
            other_keys_.push_back(std::make_shared<from_event_definition>(j));
          }

        } else if (key == "to") {
          pqrs::json::requires_array(value, "`" + key + "`");

          for (const auto& j : value) {
            to_.push_back(std::make_shared<to_event_definition>(j));
          }

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`",
                                                        key,
                                                        pqrs::json::dump_for_error_message(json)));
        }
      }
    }

    const std::vector<pqrs::not_null_shared_ptr_t<from_event_definition>>& get_other_keys(void) const {
      return other_keys_;
    }

    const to_event_definitions& get_to(void) const {
      return to_;
    }

  private:
    friend class to_if_other_key_pressed;
    std::vector<pqrs::not_null_shared_ptr_t<from_event_definition>> other_keys_;
    to_event_definitions to_;
  };

  explicit to_if_other_key_pressed(const nlohmann::json& json) {
    pqrs::json::requires_array(json, "json");

    for (const auto& j : json) {
      entries_.push_back(std::make_shared<entry>(j));
    }
  }

  virtual ~to_if_other_key_pressed(void) {
  }

  const std::vector<pqrs::not_null_shared_ptr_t<entry>>& get_entries(void) const {
    return entries_;
  }

  void setup(const event_queue::entry& front_input_event,
             std::shared_ptr<manipulated_original_event::manipulated_original_event> from_manipulated_original_event,
             std::shared_ptr<event_queue::queue> output_event_queue) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    reset();

    from_manipulated_original_event_ = from_manipulated_original_event;
    from_front_input_event_ = front_input_event;
    output_event_queue_ = output_event_queue;
  }

  void handle_from_key_up(const event_queue::entry& front_input_event) {
    if (other_key_manipulated_original_event_) {
      if (auto oeq = output_event_queue_.lock()) {
        absolute_time_duration time_stamp_delay(0);

        event_sender::post_events_at_key_up(front_input_event,
                                            *other_key_manipulated_original_event_,
                                            time_stamp_delay,
                                            *oeq);

        event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                             *other_key_manipulated_original_event_,
                                                             time_stamp_delay,
                                                             *oeq);
      }
    }

    reset();
  }

  void handle_other_key_pressed(const event_queue::entry& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    // Skip if already triggered.
    if (other_key_manipulated_original_event_) {
      return;
    }

    if (auto oeq = output_event_queue_.lock()) {
      for (const auto& entry : entries_) {
        auto other_key_from_event_definition = std::ranges::find_if(
            entry->get_other_keys(),
            [&](auto&& d) {
              return from_event_definition::test_event(front_input_event.get_event(), *d);
            });

        if (other_key_from_event_definition == std::end(entry->get_other_keys())) {
          continue;
        }

        std::shared_ptr<std::unordered_set<modifier_flag>> other_key_from_mandatory_modifiers;
        if (auto modifiers = (*other_key_from_event_definition)->get_from_modifiers_definition().test_modifiers(oeq->get_modifier_flag_manager())) {
          other_key_from_mandatory_modifiers = modifiers;
        } else {
          continue;
        }

        manipulated_original_event::from_event other_key_fe(front_input_event.get_device_id(),
                                                            front_input_event.get_event(),
                                                            front_input_event.get_original_event());
        std::vector<manipulated_original_event::from_event> other_key_from_events;

        other_key_manipulated_original_event_ =
            std::make_shared<manipulated_original_event::manipulated_original_event>(
                other_key_from_events,
                *other_key_from_mandatory_modifiers,
                front_input_event.get_event_time_stamp().get_time_stamp(),
                oeq->get_modifier_flag_manager().make_modifier_flags());

        // ----------------------------------------
        // Replace to_ events

        if (auto fmoe = from_manipulated_original_event_.lock()) {
          if (from_front_input_event_) {
            absolute_time_duration time_stamp_delay(0);

            //
            // Release from events
            //

            event_sender::post_events_at_key_up(*from_front_input_event_,
                                                *fmoe,
                                                time_stamp_delay,
                                                *oeq);

            event_sender::post_from_mandatory_modifiers_key_down(*from_front_input_event_,
                                                                 *fmoe,
                                                                 time_stamp_delay,
                                                                 *oeq);

            //
            // Press other_key_from_event_definition.to
            //

            event_sender::post_from_mandatory_modifiers_key_up(*from_front_input_event_,
                                                               *other_key_manipulated_original_event_,
                                                               front_input_event.get_event_time_stamp(),
                                                               time_stamp_delay,
                                                               *oeq);

            event_sender::post_events_at_key_down(*from_front_input_event_,
                                                  entry->get_to(),
                                                  *other_key_manipulated_original_event_,
                                                  time_stamp_delay,
                                                  *oeq);

            if (!event_sender::is_last_to_event_modifier_key_event(entry->get_to())) {
              event_sender::post_from_mandatory_modifiers_key_down(*from_front_input_event_,
                                                                   *other_key_manipulated_original_event_,
                                                                   time_stamp_delay,
                                                                   *oeq);
            }
          }
        }

        break;
      }
    }
  }

  bool needs_virtual_hid_pointing(void) const {
    for (const auto& entry : entries_) {
      if (std::ranges::any_of(entry->get_to(),
                              [](auto& e) {
                                return e->needs_virtual_hid_pointing();
                              })) {
        return true;
      }
    }
    return false;
  }

private:
  void reset(void) {
    from_manipulated_original_event_.reset();
    from_front_input_event_ = std::nullopt;
    output_event_queue_.reset();
    other_key_manipulated_original_event_.reset();
  }

  std::vector<pqrs::not_null_shared_ptr_t<entry>> entries_;
  std::weak_ptr<manipulated_original_event::manipulated_original_event> from_manipulated_original_event_;
  std::optional<event_queue::entry> from_front_input_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  std::shared_ptr<manipulated_original_event::manipulated_original_event> other_key_manipulated_original_event_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
