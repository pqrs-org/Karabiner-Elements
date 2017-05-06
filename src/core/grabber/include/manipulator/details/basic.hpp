#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include <json/json.hpp>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
class basic final : public base {
public:
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

  basic(const event_definition& from,
        const event_definition& to) : from_(from),
                                      to_({to}) {
  }

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue& event_queue,
                          size_t previous_events_size,
                          uint64_t time_stamp) {
    auto& events = event_queue.get_events();

    if (events.size() < previous_events_size) {
      return;
    }

    for (auto events_it = std::begin(events) + previous_events_size; events_it != std::end(events); ++events_it) {
      auto& queued_event = *events_it;

      if (queued_event.get_manipulated() ||
          !queued_event.get_valid()) {
        continue;
      }

      bool is_target = false;

      if (auto key_code = queued_event.get_event().get_key_code()) {
        if (from_.get_key_code() == key_code) {
          is_target = true;
        }
      }
      if (auto pointing_button = queued_event.get_event().get_pointing_button()) {
        if (from_.get_pointing_button() == pointing_button) {
          is_target = true;
        }
      }

      if (is_target) {
        if (queued_event.get_event_type() == event_type::key_down) {
          // TODO: check modifier flags

          if (!valid_) {
            is_target = false;
          }

          if (is_target) {
            manipulated_original_events_.emplace_back(queued_event.get_device_id(),
                                                      queued_event.get_original_event());
          }

        } else {
          // event_type::key_up

          // Check original_event in order to determine the correspond key_down is manipulated.

          auto it = std::find_if(std::begin(manipulated_original_events_),
                                 std::end(manipulated_original_events_),
                                 [&](const auto& pair) {
                                   return pair.first == queued_event.get_device_id() &&
                                          pair.second == queued_event.get_original_event();
                                 });
          if (it != std::end(manipulated_original_events_)) {
            manipulated_original_events_.erase(it);
          } else {
            is_target = false;
          }
        }

        if (is_target) {
          queued_event.set_valid(false);

          uint64_t to_time_stamp = queued_event.get_time_stamp();

          for (size_t i = 0; i < to_.size(); ++i) {
            if (auto event = to_[i].to_event()) {
              if (queued_event.get_event_type() == event_type::key_down) {
                events_it = events.emplace(events_it + 1,
                                           queued_event.get_device_id(),
                                           to_time_stamp + i,
                                           *event,
                                           event_type::key_down,
                                           queued_event.get_original_event());
                events_it->set_manipulated(true);

                if (i != to_.size() - 1) {
                  events_it = events.emplace(events_it + 1,
                                             queued_event.get_device_id(),
                                             to_time_stamp + i,
                                             *event,
                                             event_type::key_up,
                                             queued_event.get_original_event());
                  events_it->set_manipulated(true);
                }

              } else {
                // event_type::key_up

                if (i == to_.size() - 1) {
                  events_it = events.emplace(events_it + 1,
                                             queued_event.get_device_id(),
                                             to_time_stamp + i,
                                             *event,
                                             event_type::key_up,
                                             queued_event.get_original_event());
                  events_it->set_manipulated(true);
                }
              }
            }
          }

          for (auto it = events_it + 1; it != std::end(events); ++it) {
            it->increase_time_stamp(to_.size() - 1);
          }
        }
      }
    }
  }

  virtual bool active(void) const {
    return !manipulated_original_events_.empty();
  }

  virtual void inactivate_by_device_id(event_queue& event_queue,
                                       device_id device_id,
                                       uint64_t time_stamp) {
    while (true) {
      auto it = std::find_if(std::begin(manipulated_original_events_),
                             std::end(manipulated_original_events_),
                             [&](const auto& pair) {
                               return pair.first == device_id;
                             });
      if (it == std::end(manipulated_original_events_)) {
        break;
      }

      if (to_.size() > 0) {
        if (auto event = to_.back().to_event()) {
          event_queue.emplace_back_event(device_id,
                                         time_stamp,
                                         *event,
                                         event_type::key_up,
                                         it->second);
          event_queue.get_events().back().set_manipulated(true);
        }
      }

      manipulated_original_events_.erase(it);
    }
  }

  const event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<event_definition>& get_to(void) const {
    return to_;
  }

private:
  event_definition from_;
  std::vector<event_definition> to_;

  std::vector<std::pair<device_id, event_queue::queued_event::event>> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
