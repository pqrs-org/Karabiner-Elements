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

  explicit to_if_other_key_pressed(const nlohmann::json& json) : triggered_(false) {
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
             std::shared_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event,
             std::shared_ptr<event_queue::queue> output_event_queue) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    triggered_ = false;
    current_manipulated_original_event_ = current_manipulated_original_event;
    output_event_queue_ = output_event_queue;
  }

  void reset_if_needed(const std::shared_ptr<manipulated_original_event::manipulated_original_event>& current_manipulated_original_event) {
    if (auto cmoe = current_manipulated_original_event_.lock()) {
      if (cmoe.get() != current_manipulated_original_event.get()) {
        return;
      }

      if (!cmoe->get_from_events().empty()) {
        return;
      }
    }

    reset();
  }

  void handle_other_key_pressed(const event_queue::entry& front_input_event) {
    if (front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    if (triggered_) {
      return;
    }

    auto cmoe = current_manipulated_original_event_.lock();
    if (!cmoe) {
      reset();
      return;
    }

    if (cmoe->get_from_events().empty()) {
      reset();
      return;
    }

    for (const auto& entry : entries_) {
      if (entry->get_other_keys().empty()) {
        continue;
      }

      if (!matches_other_keys(front_input_event.get_event(),
                              entry->get_other_keys())) {
        continue;
      }

      {
        manipulated_original_event::from_event fe(front_input_event.get_device_id(),
                                                  front_input_event.get_event(),
                                                  front_input_event.get_original_event());
        if (cmoe->from_event_exists(fe)) {
          return;
        }
      }

      if (auto oeq = output_event_queue_.lock()) {
        absolute_time_duration time_stamp_delay(0);

        // ----------------------------------------
        // Replace to_ events

        event_sender::post_events_at_key_up(front_input_event,
                                            *cmoe,
                                            time_stamp_delay,
                                            *oeq);

        event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                             *cmoe,
                                                             time_stamp_delay,
                                                             *oeq);

        event_sender::post_from_mandatory_modifiers_key_up(front_input_event,
                                                           *cmoe,
                                                           time_stamp_delay,
                                                           *oeq);

        event_sender::post_events_at_key_down(front_input_event,
                                              entry->get_to(),
                                              *cmoe,
                                              time_stamp_delay,
                                              *oeq);

        if (!event_sender::is_last_to_event_modifier_key_event(entry->get_to())) {
          event_sender::post_from_mandatory_modifiers_key_down(front_input_event,
                                                               *cmoe,
                                                               time_stamp_delay,
                                                               *oeq);
        }
      }

      triggered_ = true;
      return;
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
  bool matches_other_keys(const event_queue::event& event,
                          const std::vector<pqrs::not_null_shared_ptr_t<from_event_definition>>& other_keys) const {
    for (const auto& d : other_keys) {
      if (from_event_definition::test_event(event, *d)) {
        return true;
      }
    }

    return false;
  }

  void reset(void) {
    triggered_ = false;
    current_manipulated_original_event_.reset();
    output_event_queue_.reset();
  }

  std::vector<pqrs::not_null_shared_ptr_t<entry>> entries_;
  std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  bool triggered_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
