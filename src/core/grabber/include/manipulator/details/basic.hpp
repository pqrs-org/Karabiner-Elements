#pragma once

#include "core_configuration.hpp"
#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "time_utility.hpp"
#include "logger.hpp"
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
    {
      const std::string key = "to";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          to_.emplace_back(j);
        }
      }
    }
    {
      const std::string key = "to_if_alone";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          to_if_alone_.emplace_back(j);
        }
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

  basic(const from_event_definition& from,
       const to_event_definition& to,
        vendor_id vid = vendor_id::zero,
        product_id pid = product_id::zero) : from_(from),
                                             to_({to}),
                                             vendor_id_(static_cast<uint32_t>(vid)),
                                             product_id_(static_cast<uint32_t>(pid)) {

  }

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue) {
    bool key_or_button = false;
    bool is_target = false;

    if (auto key_code = front_input_event.get_event().get_key_code()) {
      key_or_button = true;
      if (from_.get_key_code() == key_code) {
        is_target = true;
      }
    }
    if (auto pointing_button = front_input_event.get_event().get_pointing_button()) {
      key_or_button = true;
      if (from_.get_pointing_button() == pointing_button) {
        is_target = true;
      }
    }
    
    auto vp_id = get_vendor_product_id_by_device_id(front_input_event.get_device_id());
    if (!is_bypass_vendor_product_id_check() && !is_vendor_product_id_matched(vp_id.first, vp_id.second)) {
      is_target = false;
    }
  

    if (key_or_button) {
      if (front_input_event.get_event_type() == event_type::key_down) {
        for (auto& e : manipulated_original_events_) {
          e.unset_alone();
        }
      }
    }

    if (!front_input_event.get_valid()) {
      return;
    }

    if (is_target) {
      std::unordered_set<modifier_flag> from_mandatory_modifiers;
      uint64_t key_down_time_stamp = 0;
      bool alone = false;

      if (front_input_event.get_event_type() == event_type::key_down) {

        if (!valid_) {
          is_target = false;
        }

        if (auto modifiers = from_.test_modifiers(output_event_queue.get_modifier_flag_manager())) {
          from_mandatory_modifiers = *modifiers;
        } else {
          is_target = false;
        }

        if (!condition_manager_.is_fulfilled(output_event_queue.get_manipulator_environment())) {
          is_target = false;
        }

        if (is_target) {
          manipulated_original_events_.emplace_back(front_input_event.get_device_id(),
                                                    front_input_event.get_original_event(),
                                                    from_mandatory_modifiers,
                                                    front_input_event.get_time_stamp());
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
          key_down_time_stamp = it->get_key_down_time_stamp();
          alone = it->get_alone();
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
        krbn::logger::get_logger().info("Key is replaced");
        for (size_t i = 0; i < to_.size(); ++i) {
          if (auto event = to_[i].to_event()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              enqueue_to_modifiers(to_[i],
                                   event_type::key_down,
                                   front_input_event,
                                   !preserve_to_modifiers_down(),
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
                                     !preserve_to_modifiers_down(),
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
                                       !preserve_to_modifiers_down(),
                                       time_stamp_delay,
                                       output_event_queue);
                }

                uint64_t nanoseconds = time_utility::absolute_to_nano(front_input_event.get_time_stamp() - key_down_time_stamp);
                if (alone &&
                    nanoseconds < parameters_.get_basic_to_if_alone_timeout_milliseconds() * NSEC_PER_MSEC) {
                  send_to_if_alone(front_input_event,
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

  virtual void handle_event_from_ignored_device(event_queue::queued_event::event::type original_type,
                                                int64_t original_integer_value,
                                                event_type event_type,
                                                event_queue& output_event_queue,
                                                uint64_t time_stamp) {
    bool need_to_unset_alone = false;

    if (original_type == event_queue::queued_event::event::type::key_code ||
        original_type == event_queue::queued_event::event::type::pointing_button) {
      if (event_type == event_type::key_down) {
        need_to_unset_alone = true;
      }
    }
    if (original_type == event_queue::queued_event::event::type::pointing_vertical_wheel ||
        original_type == event_queue::queued_event::event::type::pointing_horizontal_wheel) {
      if (original_integer_value != 0) {
        need_to_unset_alone = true;
      }
    }

    if (need_to_unset_alone) {
      for (auto& e : manipulated_original_events_) {
        e.unset_alone();
      }
    }
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }
  
private:
  bool is_bypass_vendor_product_id_check();
  bool is_vendor_product_id_matched(uint32_t vendor_id, uint32_t product_id);

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

  void send_to_if_alone(const event_queue::queued_event& front_input_event,
                        uint64_t& time_stamp_delay,
                        event_queue& output_event_queue) {
    for (const auto& to : to_if_alone_) {
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
  
  std::pair<uint32_t, uint32_t> get_vendor_product_id_by_device_id(krbn::device_id id);
  
  core_configuration::profile::complex_modifications::parameters parameters_;

  from_event_definition from_;
  
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_if_alone_;
  uint32_t vendor_id_;
  uint32_t product_id_;

  std::vector<manipulated_original_event> manipulated_original_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
