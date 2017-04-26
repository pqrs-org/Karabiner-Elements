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

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue& event_queue, std::chrono::nanoseconds time) {
    auto& events = event_queue.get_events();

    for (auto it = std::begin(events); it != std::end(events); ++it) {
      auto& queued_event = *it;

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

          manipulated_original_events_.emplace_back(queued_event.get_device_id(),
                                                    queued_event.get_original_event());

        } else {
          // event_type::key_up

          // Check original_event in order to determine the correspond key_down is manipulated.

          auto it = std::find_if(std::begin(manipulated_original_events_),
                                 std::end(manipulated_original_events_),
                                 [&](const auto& pair) {
                                   return pair.first == queued_event.get_device_id() &&
                                          pair.second == queued_event.get_original_event();
                                 });
          if (it == std::end(manipulated_original_events_)) {
            is_target = false;
          }

          manipulated_original_events_.erase(it);
        }

        if (is_target) {
          queued_event.set_valid(false);

          for (size_t i = 0; i < to_.size(); ++i) {
            if (auto event = to_[i].to_event()) {
              if (queued_event.get_event_type() == event_type::key_down) {
                it = events.emplace(it,
                                    queued_event.get_device_id(),
                                    queued_event.get_time_stamp(),
                                    *event,
                                    event_type::key_down,
                                    queued_event.get_original_event());

                if (i != to_.size() - 1) {
                  it = events.emplace(it,
                                      queued_event.get_device_id(),
                                      queued_event.get_time_stamp(),
                                      *event,
                                      event_type::key_up,
                                      queued_event.get_original_event());
                }

              } else {
                // event_type::key_up

                if (i == to_.size() - 1) {
                  it = events.emplace(it,
                                      queued_event.get_device_id(),
                                      queued_event.get_time_stamp(),
                                      *event,
                                      event_type::key_up,
                                      queued_event.get_original_event());
                }
              }
            }
          }
        }
      }
    }
  }

  virtual bool active(void) const {
    return !manipulated_original_events_.empty();
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
