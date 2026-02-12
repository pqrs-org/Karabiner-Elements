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
    const std::vector<from_event_definition>& get_other_keys(void) const {
      return other_keys_;
    }

    const to_event_definitions& get_to(void) const {
      return to_;
    }

  private:
    friend class to_if_other_key_pressed;
    std::vector<from_event_definition> other_keys_;
    to_event_definitions to_;
  };

  to_if_other_key_pressed(const nlohmann::json& json) : triggered_(false) {
    if (json.is_object()) {
      entries_.push_back(parse_entry(json));
    } else if (json.is_array()) {
      for (const auto& j : json) {
        try {
          entries_.push_back(parse_entry(j));
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("entry error: {0}", e.what()));
        }
      }
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object or array, but is `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  }

  virtual ~to_if_other_key_pressed(void) {
  }

  const std::vector<entry>& get_entries(void) const {
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
      if (entry.get_other_keys().empty() || entry.get_to().empty()) {
        continue;
      }

      if (!matches_other_keys(front_input_event.get_event(), entry.get_other_keys())) {
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
                                              entry.get_to(),
                                              *cmoe,
                                              time_stamp_delay,
                                              *oeq);

        if (!event_sender::is_last_to_event_modifier_key_event(entry.get_to())) {
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
      if (std::any_of(std::begin(entry.get_to()),
                      std::end(entry.get_to()),
                      [](auto& e) {
                        return e->needs_virtual_hid_pointing();
                      })) {
        return true;
      }
    }
    return false;
  }

private:
  entry parse_entry(const nlohmann::json& json) {
    pqrs::json::requires_object(json, "json");

    entry result;

    for (const auto& [key, value] : json.items()) {
      if (key == "other_keys") {
        if (value.is_object()) {
          add_other_key(result.other_keys_, value);

        } else if (value.is_array()) {
          for (const auto& j : value) {
            add_other_key(result.other_keys_, j);
          }

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, pqrs::json::dump_for_error_message(value)));
        }

      } else if (key == "to") {
        if (value.is_object()) {
          try {
            result.to_.push_back(std::make_shared<to_event_definition>(value));
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
          }

        } else if (value.is_array()) {
          try {
            for (const auto& j : value) {
              result.to_.push_back(std::make_shared<to_event_definition>(j));
            }
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
          }

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array, but is `{1}`", key, pqrs::json::dump_for_error_message(value)));
        }

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }

    return result;
  }

  void add_other_key(std::vector<from_event_definition>& other_keys,
                     const nlohmann::json& json) {
    try {
      other_keys.push_back(json.get<from_event_definition>());
    } catch (const pqrs::json::unmarshal_error& e) {
      throw pqrs::json::unmarshal_error(fmt::format("`other_keys` entry error: {0}", e.what()));
    }
  }

  bool matches_other_keys(const event_queue::event& event,
                          const std::vector<from_event_definition>& other_keys) const {
    for (const auto& d : other_keys) {
      if (from_event_definition::test_event(event, d)) {
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

  std::vector<entry> entries_;
  std::weak_ptr<manipulated_original_event::manipulated_original_event> current_manipulated_original_event_;
  std::weak_ptr<event_queue::queue> output_event_queue_;
  bool triggered_;
};
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
