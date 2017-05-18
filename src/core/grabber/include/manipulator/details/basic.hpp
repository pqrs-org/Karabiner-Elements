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
                               const std::unordered_set<modifier_flag> from_modifiers) : device_id_(device_id),
                                                                                         original_event_(original_event),
                                                                                         from_modifiers_(from_modifiers) {
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    const event_queue::queued_event::event& get_original_event(void) const {
      return original_event_;
    }

    const std::unordered_set<modifier_flag>& get_from_modifiers(void) const {
      return from_modifiers_;
    }

    bool operator==(const manipulated_original_event& other) const {
      // Do not compare `from_modifiers_`.
      return get_device_id() == other.get_device_id() &&
             get_original_event() == other.get_original_event();
    }

  private:
    device_id device_id_;
    event_queue::queued_event::event original_event_;
    std::unordered_set<modifier_flag> from_modifiers_;
  };

  basic(const nlohmann::json& json) : base(),
                                      from_(json.find("from") != std::end(json) ? json["from"] : nlohmann::json()) {
    const std::string key = "to";
    if (json.find(key) != std::end(json) && json[key].is_array()) {
      for (const auto& j : json[key]) {
        to_.emplace_back(j);
      }
    }
                                         
    if (json.find("vendor_id") != std::end(json)) {
      vendor_id_ = json["vendor_id"];
    } else {
      vendor_id_ = 0;
    }
   
    if (json.find("product_id") != std::end(json)) {
      product_id_ = json["product_id"];
    } else {
      product_id_ = 0;
    }
  }

  basic(const event_definition& from,
        const event_definition& to, vendor_id vid, product_id pid) : from_(from),
                                                                     to_({to}),
                                                                     vendor_id_(static_cast<uint32_t>(vid)),
                                                                     product_id_(static_cast<uint32_t>(pid)) {
  }

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp);

  virtual bool active(void) const {
    return !manipulated_original_events_.empty();
  }

  virtual void inactivate_by_device_id(device_id device_id,
                                       event_queue& output_event_queue,
                                       uint64_t time_stamp) {
    while (true) {
      auto it = std::find_if(std::begin(manipulated_original_events_),
                             std::end(manipulated_original_events_),
                             [&](const auto& manipulated_original_event) {
                               return manipulated_original_event.get_device_id() == device_id;
                             });
      if (it == std::end(manipulated_original_events_)) {
        break;
      }

      if (to_.size() > 0) {
        if (auto event = to_.back().to_event()) {
          output_event_queue.emplace_back_event(device_id,
                                                time_stamp,
                                                *event,
                                                event_type::key_up,
                                                it->get_original_event());
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
  bool is_bypass_vendor_product_id_check();
  bool is_vendor_product_id_matched(uint32_t vendor_id, uint32_t product_id);

private:
  event_definition from_;
  std::vector<event_definition> to_;
  uint32_t vendor_id_;
  uint32_t product_id_;

  std::vector<manipulated_original_event> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
