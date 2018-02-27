#pragma once

#include "core_configuration.hpp"
#include "krbn_notification_center.hpp"
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
  class from_event_definition final {
  public:
    class simultaneous_options final {
    public:
      enum class key_order {
        insensitive,
        strict,
        strict_inverse,
      };

      simultaneous_options(void) : key_down_order_(key_order::insensitive),
                                   key_up_order_(key_order::insensitive) {
      }

      void handle_json(const nlohmann::json& json) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "key_down_order") {
            if (!value.is_string()) {
              logger::get_logger().error("complex_modifications json error: `key_down_order` should be string: {0}", json.dump());
              continue;
            }

            key_down_order_ = value;

            continue;
          }

          if (key == "key_up_order") {
            if (!value.is_string()) {
              logger::get_logger().error("complex_modifications json error: `key_up_order` should be string: {0}", json.dump());
              continue;
            }

            key_up_order_ = value;

            continue;
          }

          if (key == "to_after_key_up") {
            if (!value.is_array()) {
              logger::get_logger().error("complex_modifications json error: `to_after_key_up` should be array: {0}", json.dump());
              continue;
            }

            for (const auto& j : value) {
              to_after_key_up_.emplace_back(j);
            }

            continue;
          }

          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }

      key_order get_key_down_order(void) const {
        return key_down_order_;
      }

      key_order get_key_up_order(void) const {
        return key_up_order_;
      }

      const std::vector<to_event_definition>& get_to_after_key_up(void) const {
        return to_after_key_up_;
      }

    private:
      key_order key_down_order_;
      key_order key_up_order_;
      std::vector<to_event_definition> to_after_key_up_;
    };

    from_event_definition(const nlohmann::json& json) {
      if (!json.is_object()) {
        logger::get_logger().error("complex_modifications json error: Invalid form of from_event_definition: {0}", json.dump());
        return;
      }

      event_definition default_event_definition;

      for (auto it1 = std::begin(json); it1 != std::end(json); std::advance(it1, 1)) {
        // it1.key() is always std::string.
        const auto& key = it1.key();
        const auto& value = it1.value();

        if (default_event_definition.handle_json(key, value, json)) {
          continue;
        }

        if (key == "simultaneous") {
          if (!value.is_array()) {
            logger::get_logger().error("complex_modifications json error: Invalid form of simultaneous: {0}", value.dump());
            continue;
          }

          for (const auto& j : value) {
            event_definition d;

            for (auto it2 = std::begin(j); it2 != std::end(j); std::advance(it2, 1)) {
              // it.key() is always std::string.
              const auto& k = it2.key();
              const auto& v = it2.value();

              d.handle_json(k, v, j);
            }

            if (d.get_type() != event_definition::type::none) {
              event_definitions_.push_back(d);
            }
          }

          continue;
        }

        if (key == "simultaneous_options") {
          if (!value.is_object()) {
            logger::get_logger().error("complex_modifications json error: Invalid form of simultaneous_options: {0}", value.dump());
            continue;
          }

          simultaneous_options_.handle_json(value);

          continue;
        }

        if (key == "modifiers") {
          if (!value.is_object()) {
            logger::get_logger().error("complex_modifications json error: Invalid form of modifiers: {0}", value.dump());
            continue;
          }

          for (auto it2 = std::begin(value); it2 != std::end(value); std::advance(it2, 1)) {
            // it.key() is always std::string.
            const auto& k = it2.key();
            const auto& v = it2.value();

            if (k == "mandatory") {
              mandatory_modifiers_ = modifier_definition::make_modifiers(v);
            } else if (k == "optional") {
              optional_modifiers_ = modifier_definition::make_modifiers(v);
            } else {
              logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", k, value.dump());
            }
          }

          continue;
        }

        logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
      }

      if (event_definitions_.empty() &&
          default_event_definition.get_type() != event_definition::type::none) {
        event_definitions_.push_back(default_event_definition);
      }

      // ----------------------------------------

      for (const auto& d : event_definitions_) {
        switch (d.get_type()) {
          case event_definition::type::key_code:
          case event_definition::type::consumer_key_code:
          case event_definition::type::pointing_button:
          case event_definition::type::any:
            break;

          case event_definition::type::none:
          case event_definition::type::shell_command:
          case event_definition::type::select_input_source:
          case event_definition::type::set_variable:
          case event_definition::type::mouse_key:
            logger::get_logger().error("complex_modifications json error: Invalid type in from_event_definition: {0}", json.dump());
            break;
        }
      }
    }

    virtual ~from_event_definition(void) {
    }

    const std::vector<event_definition>& get_event_definitions(void) const {
      return event_definitions_;
    }

    const std::unordered_set<modifier_definition::modifier>& get_mandatory_modifiers(void) const {
      return mandatory_modifiers_;
    }

    const std::unordered_set<modifier_definition::modifier>& get_optional_modifiers(void) const {
      return optional_modifiers_;
    }

    const simultaneous_options& get_simultaneous_options(void) const {
      return simultaneous_options_;
    }

    boost::optional<std::unordered_set<modifier_flag>> test_modifiers(const modifier_flag_manager& modifier_flag_manager) const {
      std::unordered_set<modifier_flag> modifier_flags;

      // If mandatory_modifiers_ contains modifier::any, return all active modifier_flags.

      if (mandatory_modifiers_.find(modifier_definition::modifier::any) != std::end(mandatory_modifiers_)) {
        for (auto i = static_cast<uint32_t>(modifier_flag::zero) + 1; i != static_cast<uint32_t>(modifier_flag::end_); ++i) {
          auto flag = modifier_flag(i);
          if (modifier_flag_manager.is_pressed(flag)) {
            modifier_flags.insert(flag);
          }
        }
        return modifier_flags;
      }

      // Check modifier_flag state.

      for (int i = 0; i < static_cast<int>(modifier_definition::modifier::end_); ++i) {
        auto m = modifier_definition::modifier(i);

        if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_)) {
          auto pair = test_modifier(modifier_flag_manager, m);
          if (!pair.first) {
            return boost::none;
          }
          if (pair.second != modifier_flag::zero) {
            modifier_flags.insert(pair.second);
          }
        }
      }

      // If optional_modifiers_ does not contain modifier::any, we have to check modifier flags strictly.

      if (optional_modifiers_.find(modifier_definition::modifier::any) == std::end(optional_modifiers_)) {
        std::unordered_set<modifier_flag> extra_modifier_flags;
        for (auto m = static_cast<uint32_t>(modifier_flag::zero) + 1; m != static_cast<uint32_t>(modifier_flag::end_); ++m) {
          extra_modifier_flags.insert(modifier_flag(m));
        }

        for (int i = 0; i < static_cast<int>(modifier_definition::modifier::end_); ++i) {
          auto m = modifier_definition::modifier(i);

          if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_) ||
              optional_modifiers_.find(m) != std::end(optional_modifiers_)) {
            for (const auto& flag : modifier_definition::get_modifier_flags(m)) {
              extra_modifier_flags.erase(flag);
            }
          }
        }

        for (const auto& flag : extra_modifier_flags) {
          if (modifier_flag_manager.is_pressed(flag)) {
            return boost::none;
          }
        }
      }

      return modifier_flags;
    }

    static std::pair<bool, modifier_flag> test_modifier(const modifier_flag_manager& modifier_flag_manager,
                                                        modifier_definition::modifier modifier) {
      if (modifier == modifier_definition::modifier::any) {
        return std::make_pair(true, modifier_flag::zero);
      }

      auto modifier_flags = modifier_definition::get_modifier_flags(modifier);
      if (!modifier_flags.empty()) {
        for (const auto& m : modifier_flags) {
          if (modifier_flag_manager.is_pressed(m)) {
            return std::make_pair(true, m);
          }
        }
      }

      return std::make_pair(false, modifier_flag::zero);
    }

    static bool test_event(const event_queue::queued_event::event& event,
                           const event_definition& event_definition) {
      if (auto key_code = event.get_key_code()) {
        if (event_definition.get_key_code() == key_code ||
            event_definition.get_any_type() == event_definition::type::key_code) {
          return true;
        }
      }

      if (auto consumer_key_code = event.get_consumer_key_code()) {
        if (event_definition.get_consumer_key_code() == consumer_key_code ||
            event_definition.get_any_type() == event_definition::type::consumer_key_code) {
          return true;
        }
      }

      if (auto pointing_button = event.get_pointing_button()) {
        if (event_definition.get_pointing_button() == pointing_button ||
            event_definition.get_any_type() == event_definition::type::pointing_button) {
          return true;
        }
      }

      return false;
    }

    static bool test_event(const event_queue::queued_event::event& event,
                           const from_event_definition& from_event_definition) {
      for (const auto& d : from_event_definition.get_event_definitions()) {
        if (test_event(event, d)) {
          return true;
        }
      }

      return false;
    }

    static bool test_key_order(const std::vector<event_queue::queued_event::event>& events,
                               simultaneous_options::key_order key_order,
                               const std::vector<event_definition>& event_definitions) {
      switch (key_order) {
        case simultaneous_options::key_order::insensitive:
          // Do nothing
          break;

        case simultaneous_options::key_order::strict:
          for (auto events_it = std::begin(events); events_it != std::end(events); std::advance(events_it, 1)) {
            auto event_definitions_index = static_cast<size_t>(std::distance(std::begin(events), events_it));
            if (event_definitions_index < event_definitions.size()) {
              if (!test_event(*events_it, event_definitions[event_definitions_index])) {
                return false;
              }
            }
          }
          break;

        case simultaneous_options::key_order::strict_inverse:
          for (auto events_it = std::begin(events); events_it != std::end(events); std::advance(events_it, 1)) {
            auto event_definitions_index = static_cast<size_t>(std::distance(std::begin(events), events_it));
            if (event_definitions_index < event_definitions.size()) {
              if (!test_event(*events_it, event_definitions[event_definitions.size() - 1 - event_definitions_index])) {
                return false;
              }
            }
          }
          break;
      }

      return true;
    }

  private:
    std::vector<event_definition> event_definitions_;
    std::unordered_set<modifier_definition::modifier> mandatory_modifiers_;
    std::unordered_set<modifier_definition::modifier> optional_modifiers_;
    simultaneous_options simultaneous_options_;
  };

  class manipulated_original_event final {
  public:
    class from_event final {
    public:
      from_event(void) : device_id_(device_id::zero) {
      }

      from_event(device_id device_id,
                 const event_queue::queued_event::event& event,
                 const event_queue::queued_event::event& original_event) : device_id_(device_id),
                                                                           event_(event),
                                                                           original_event_(original_event) {
      }

      device_id get_device_id(void) const {
        return device_id_;
      }

      const event_queue::queued_event::event& get_event(void) const {
        return event_;
      }

      const event_queue::queued_event::event& get_original_event(void) const {
        return original_event_;
      }

      bool operator==(const from_event& other) const {
        return device_id_ == other.device_id_ &&
               event_ == other.event_ &&
               original_event_ == other.original_event_;
      }

      friend size_t hash_value(const from_event& value) {
        size_t h = 0;
        boost::hash_combine(h, value.device_id_);
        boost::hash_combine(h, value.event_);
        boost::hash_combine(h, value.original_event_);
        return h;
      }

    private:
      device_id device_id_;
      event_queue::queued_event::event event_;
      event_queue::queued_event::event original_event_;
    };

    struct from_event_hash final {
      std::size_t operator()(const from_event& v) const noexcept {
        return hash_value(v);
      }
    };

    class events_at_key_up final {
    public:
      class entry {
      public:
        entry(device_id device_id,
              const event_queue::queued_event::event& event,
              event_type event_type,
              const event_queue::queued_event::event& original_event,
              bool lazy) : device_id_(device_id),
                           event_(event),
                           event_type_(event_type),
                           original_event_(original_event),
                           lazy_(lazy) {
        }

        event_queue::queued_event make_queued_event(const event_queue::queued_event::event_time_stamp& event_time_stamp) const {
          return event_queue::queued_event(device_id_,
                                           event_time_stamp,
                                           event_,
                                           event_type_,
                                           original_event_,
                                           lazy_);
        }

      private:
        device_id device_id_;
        event_queue::queued_event::event event_;
        event_type event_type_;
        event_queue::queued_event::event original_event_;
        bool lazy_;
      };

      const std::vector<entry>& get_events(void) const {
        return events_;
      }

      void emplace_back_event(device_id device_id,
                              const event_queue::queued_event::event& event,
                              event_type event_type,
                              const event_queue::queued_event::event& original_event,
                              bool lazy) {
        events_.emplace_back(device_id,
                             event,
                             event_type,
                             original_event,
                             lazy);
      }

      void clear_events(void) {
        events_.clear();
      }

    private:
      std::vector<entry> events_;
    };

    manipulated_original_event(const std::unordered_set<from_event, from_event_hash>& from_events,
                               const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
                               uint64_t key_down_time_stamp) : from_events_(from_events),
                                                               from_mandatory_modifiers_(from_mandatory_modifiers),
                                                               from_mandatory_modifiers_restored_(false),
                                                               key_down_time_stamp_(key_down_time_stamp),
                                                               alone_(true),
                                                               key_up_posted_(false) {
    }

    const std::unordered_set<from_event, from_event_hash>& get_from_events(void) const {
      return from_events_;
    }

    const std::unordered_set<modifier_flag>& get_from_mandatory_modifiers(void) const {
      return from_mandatory_modifiers_;
    }

    bool get_from_mandatory_modifiers_restored(void) const {
      return from_mandatory_modifiers_restored_;
    }

    void set_from_mandatory_modifiers_restored(bool value) {
      from_mandatory_modifiers_restored_ = value;
    }

    uint64_t get_key_down_time_stamp(void) const {
      return key_down_time_stamp_;
    }

    bool get_alone(void) const {
      return alone_;
    }

    const events_at_key_up& get_events_at_key_up(void) const {
      return events_at_key_up_;
    }
    events_at_key_up& get_events_at_key_up(void) {
      return const_cast<events_at_key_up&>(static_cast<const manipulated_original_event&>(*this).get_events_at_key_up());
    }

    bool get_key_up_posted(void) const {
      return key_up_posted_;
    }

    void set_key_up_posted(bool value) {
      key_up_posted_ = true;
    }

    void unset_alone(void) {
      alone_ = false;
    }

    bool from_event_exists(const from_event& from_event) const {
      return from_events_.find(from_event) != std::end(from_events_);
    }

    void erase_from_event(const from_event& from_event) {
      from_events_.erase(from_event);
    }

    void erase_from_events_by_device_id(device_id device_id) {
      for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
        if (it->get_device_id() == device_id) {
          it = from_events_.erase(it);
        } else {
          std::advance(it, 1);
        }
      }
    }

    void erase_from_events_by_event(const event_queue::queued_event::event& event) {
      for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
        if (it->get_event() == event) {
          it = from_events_.erase(it);
        } else {
          std::advance(it, 1);
        }
      }
    }

  private:
    std::unordered_set<from_event, from_event_hash> from_events_;
    std::unordered_set<modifier_flag> from_mandatory_modifiers_;
    bool from_mandatory_modifiers_restored_;
    uint64_t key_down_time_stamp_;
    bool alone_;
    events_at_key_up events_at_key_up_;
    bool key_up_posted_;
  };

  class to_if_held_down final {
  public:
    to_if_held_down(basic& basic,
                    const nlohmann::json& json) : basic_(basic) {
      if (json.is_array()) {
        for (const auto& j : json) {
          to_.emplace_back(j);
        }
      } else {
        logger::get_logger().error("complex_modifications json error: `to_if_held_down` should be object: {0}", json.dump());
      }
    }

    void setup(const event_queue::queued_event& front_input_event,
               const std::shared_ptr<manipulated_original_event>& current_manipulated_original_event,
               const std::shared_ptr<event_queue>& output_event_queue,
               int threshold_milliseconds) {
      if (front_input_event.get_event_type() != event_type::key_down) {
        manipulator_timer_id_ = boost::none;
        return;
      }

      if (to_.empty()) {
        return;
      }

      front_input_event_ = front_input_event;
      current_manipulated_original_event_ = current_manipulated_original_event;
      output_event_queue_ = output_event_queue;

      auto when = front_input_event.get_event_time_stamp().get_time_stamp() + time_utility::nano_to_absolute(threshold_milliseconds * NSEC_PER_MSEC);
      manipulator_timer_id_ = manipulator_timer::get_instance().add_entry(when);
    }

    void cancel(const event_queue::queued_event& front_input_event) {
      if (front_input_event.get_event_type() != event_type::key_down) {
        return;
      }

      if (!manipulator_timer_id_) {
        return;
      }

      manipulator_timer_id_ = boost::none;
    }

    void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
      if (timer_id == manipulator_timer_id_) {
        manipulator_timer_id_ = boost::none;

        if (front_input_event_) {
          if (auto oeq = output_event_queue_.lock()) {
            if (auto cmoe = current_manipulated_original_event_.lock()) {
              uint64_t time_stamp_delay = 0;

              // ----------------------------------------
              // Post events_at_key_up_

              basic_.post_events_at_key_up(*front_input_event_,
                                           *cmoe,
                                           time_stamp_delay,
                                           *oeq);

              basic_.post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                            *cmoe,
                                                            time_stamp_delay,
                                                            *oeq);

              // ----------------------------------------
              // Post to_

              basic_.post_from_mandatory_modifiers_key_up(*front_input_event_,
                                                          *cmoe,
                                                          time_stamp_delay,
                                                          *oeq);

              basic_.post_events_at_key_down(*front_input_event_,
                                             to_,
                                             *cmoe,
                                             time_stamp_delay,
                                             *oeq);

              if (!basic_.is_last_to_event_modifier_key_event(to_)) {
                basic_.post_from_mandatory_modifiers_key_down(*front_input_event_,
                                                              *cmoe,
                                                              time_stamp_delay,
                                                              *oeq);
              }

              // ----------------------------------------

              krbn_notification_center::get_instance().input_event_arrived();
            }
          }
        }
      }
    }

    bool needs_virtual_hid_pointing(void) const {
      return std::any_of(std::begin(to_),
                         std::end(to_),
                         [](auto& e) {
                           return e.needs_virtual_hid_pointing();
                         });
    }

  private:
    basic& basic_;
    std::vector<to_event_definition> to_;
    boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
    boost::optional<event_queue::queued_event> front_input_event_;
    std::weak_ptr<manipulated_original_event> current_manipulated_original_event_;
    std::weak_ptr<event_queue> output_event_queue_;
  };

  class to_delayed_action final {
  public:
    to_delayed_action(basic& basic,
                      const nlohmann::json& json) : basic_(basic) {
      if (json.is_object()) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "to_if_invoked") {
            if (!value.is_array()) {
              logger::get_logger().error("complex_modifications json error: `to_if_invoked` should be array: {0}", json.dump());
              continue;
            }

            for (const auto& j : value) {
              to_if_invoked_.emplace_back(j);
            }

          } else if (key == "to_if_canceled") {
            if (!value.is_array()) {
              logger::get_logger().error("complex_modifications json error: `to_if_canceled` should be array: {0}", json.dump());
              continue;
            }

            for (const auto& j : value) {
              to_if_canceled_.emplace_back(j);
            }

          } else {
            logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
          }
        }
      } else {
        logger::get_logger().error("complex_modifications json error: `to_delayed_action` should be object: {0}", json.dump());
      }
    }

    void setup(const event_queue::queued_event& front_input_event,
               const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
               const std::shared_ptr<event_queue>& output_event_queue,
               int delay_milliseconds) {
      if (front_input_event.get_event_type() != event_type::key_down) {
        return;
      }

      if (to_if_invoked_.empty() &&
          to_if_canceled_.empty()) {
        return;
      }

      front_input_event_ = front_input_event;
      from_mandatory_modifiers_ = from_mandatory_modifiers;
      output_event_queue_ = output_event_queue;

      auto when = front_input_event.get_event_time_stamp().get_time_stamp() + time_utility::nano_to_absolute(delay_milliseconds * NSEC_PER_MSEC);
      manipulator_timer_id_ = manipulator_timer::get_instance().add_entry(when);
    }

    void cancel(const event_queue::queued_event& front_input_event) {
      if (front_input_event.get_event_type() != event_type::key_down) {
        return;
      }

      if (!manipulator_timer_id_) {
        return;
      }

      manipulator_timer_id_ = boost::none;

      post_events(to_if_canceled_);
    }

    void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
      if (timer_id == manipulator_timer_id_) {
        manipulator_timer_id_ = boost::none;
        post_events(to_if_invoked_);
        krbn_notification_center::get_instance().input_event_arrived();
      }
    }

    bool needs_virtual_hid_pointing(void) const {
      for (const auto& events : {to_if_invoked_,
                                 to_if_canceled_}) {
        if (std::any_of(std::begin(events),
                        std::end(events),
                        [](auto& e) {
                          return e.needs_virtual_hid_pointing();
                        })) {
          return true;
        }
      }
      return false;
    }

  private:
    void post_events(const std::vector<to_event_definition>& events) const {
      if (front_input_event_) {
        if (auto oeq = output_event_queue_.lock()) {
          uint64_t time_stamp_delay = 0;

          // Release from_mandatory_modifiers

          basic_.post_lazy_modifier_key_events(*front_input_event_,
                                               from_mandatory_modifiers_,
                                               event_type::key_up,
                                               time_stamp_delay,
                                               *oeq);

          // Post events

          basic_.post_extra_to_events(*front_input_event_,
                                      events,
                                      time_stamp_delay,
                                      *oeq);

          // Restore from_mandatory_modifiers

          basic_.post_lazy_modifier_key_events(*front_input_event_,
                                               from_mandatory_modifiers_,
                                               event_type::key_down,
                                               time_stamp_delay,
                                               *oeq);
        }
      }
    }

    basic& basic_;
    std::vector<to_event_definition> to_if_invoked_;
    std::vector<to_event_definition> to_if_canceled_;
    boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
    boost::optional<event_queue::queued_event> front_input_event_;
    std::unordered_set<modifier_flag> from_mandatory_modifiers_;
    std::weak_ptr<event_queue> output_event_queue_;
  };

  basic(const nlohmann::json& json,
        const core_configuration::profile::complex_modifications::parameters& parameters) : base(),
                                                                                            parameters_(parameters),
                                                                                            from_(json_utility::find_copy(json, "from", nlohmann::json())) {
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

      } else if (key == "to_if_held_down") {
        to_if_held_down_ = std::make_unique<to_if_held_down>(*this, value);

      } else if (key == "to_delayed_action") {
        to_delayed_action_ = std::make_unique<to_delayed_action>(*this, value);

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

  virtual bool already_manipulated(const event_queue::queued_event& front_input_event) {
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

  virtual manipulate_result manipulate(event_queue::queued_event& front_input_event,
                                       const event_queue& input_event_queue,
                                       const std::shared_ptr<event_queue>& output_event_queue,
                                       uint64_t now) {
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
        std::shared_ptr<manipulated_original_event> current_manipulated_original_event;

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
                if (auto modifiers = from_.test_modifiers(output_event_queue->get_modifier_flag_manager())) {
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

              std::unordered_set<manipulated_original_event::from_event, manipulated_original_event::from_event_hash> from_events;

              {
                std::vector<event_queue::queued_event::event> ordered_key_down_events;
                std::vector<event_queue::queued_event::event> ordered_key_up_events;
                uint64_t simultaneous_threshold_milliseconds = parameters_.get_basic_simultaneous_threshold_milliseconds();
                uint64_t end_time_stamp = front_input_event.get_event_time_stamp().get_time_stamp() + time_utility::nano_to_absolute(simultaneous_threshold_milliseconds * NSEC_PER_MSEC);

                for (const auto& queued_event : input_event_queue.get_events()) {
                  if (!is_target) {
                    break;
                  }

                  if (!queued_event.get_valid()) {
                    continue;
                  }

                  if (end_time_stamp < queued_event.get_event_time_stamp().get_time_stamp()) {
                    break;
                  }

                  manipulated_original_event::from_event fe(queued_event.get_device_id(),
                                                            queued_event.get_event(),
                                                            queued_event.get_original_event());

                  switch (queued_event.get_event_type()) {
                    case event_type::key_down:
                      if (from_event_definition::test_event(queued_event.get_event(), from_)) {
                        // Insert the first event if the same events are arrived from different device.

                        if (std::none_of(std::begin(from_events),
                                         std::end(from_events),
                                         [&](auto& e) {
                                           return fe.get_event() == e.get_event();
                                         })) {
                          from_events.insert(fe);
                          ordered_key_down_events.push_back(queued_event.get_event());
                        }

                      } else {
                        // Do not manipulate if another event arrived.

                        if (!all_from_events_found(from_events)) {
                          is_target = false;
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
                                             return e == queued_event.get_event();
                                           })) {
                            ordered_key_up_events.push_back(queued_event.get_event());
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
                    case from_event_definition::simultaneous_options::key_order::insensitive:
                      // Do nothing
                      break;

                    case from_event_definition::simultaneous_options::key_order::strict:
                    case from_event_definition::simultaneous_options::key_order::strict_inverse:
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
                // Update input_delay_time_stamp

                if (is_target) {
                  auto found = all_from_events_found(from_events);
                  if (needs_wait_key_up || !found) {
                    auto t = std::max(front_input_event.get_event_time_stamp().get_input_delay_time_stamp(),
                                      time_utility::nano_to_absolute(simultaneous_threshold_milliseconds * NSEC_PER_MSEC));
                    front_input_event.get_event_time_stamp().set_input_delay_time_stamp(t);

                    if (now < front_input_event.get_event_time_stamp().make_time_stamp_with_input_delay()) {
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
                  current_manipulated_original_event = std::make_shared<manipulated_original_event>(from_events,
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

          uint64_t time_stamp_delay = 0;

          // Send events

          switch (front_input_event.get_event_type()) {
            case event_type::key_down:
              // Release from_mandatory_modifiers

              post_from_mandatory_modifiers_key_up(front_input_event,
                                                   *current_manipulated_original_event,
                                                   time_stamp_delay,
                                                   *output_event_queue);

              post_events_at_key_down(front_input_event,
                                      to_,
                                      *current_manipulated_original_event,
                                      time_stamp_delay,
                                      *output_event_queue);

              if (!is_last_to_event_modifier_key_event(to_)) {
                post_from_mandatory_modifiers_key_down(front_input_event,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);
              }

              break;

            case event_type::key_up:
              if (!current_manipulated_original_event->get_key_up_posted()) {
                current_manipulated_original_event->set_key_up_posted(true);

                // to_

                post_events_at_key_up(front_input_event,
                                      *current_manipulated_original_event,
                                      time_stamp_delay,
                                      *output_event_queue);

                post_from_mandatory_modifiers_key_down(front_input_event,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);

                // to_after_key_up_

                if (!to_after_key_up_.empty()) {
                  post_from_mandatory_modifiers_key_up(front_input_event,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);

                  post_extra_to_events(front_input_event,
                                       to_after_key_up_,
                                       time_stamp_delay,
                                       *output_event_queue);

                  post_from_mandatory_modifiers_key_down(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);
                }

                // to_if_alone_

                if (!to_if_alone_.empty()) {
                  uint64_t nanoseconds = time_utility::absolute_to_nano(front_input_event.get_event_time_stamp().get_time_stamp() - current_manipulated_original_event->get_key_down_time_stamp());
                  if (current_manipulated_original_event->get_alone() &&
                      nanoseconds < parameters_.get_basic_to_if_alone_timeout_milliseconds() * NSEC_PER_MSEC) {
                    post_from_mandatory_modifiers_key_up(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);

                    post_extra_to_events(front_input_event,
                                         to_if_alone_,
                                         time_stamp_delay,
                                         *output_event_queue);

                    post_from_mandatory_modifiers_key_down(front_input_event,
                                                           *current_manipulated_original_event,
                                                           time_stamp_delay,
                                                           *output_event_queue);
                  }
                }
              }

              // simultaneous_options.to_after_key_up_

              if (!from_.get_simultaneous_options().get_to_after_key_up().empty()) {
                if (current_manipulated_original_event->get_from_events().empty()) {
                  post_from_mandatory_modifiers_key_up(front_input_event,
                                                       *current_manipulated_original_event,
                                                       time_stamp_delay,
                                                       *output_event_queue);

                  post_extra_to_events(front_input_event,
                                       from_.get_simultaneous_options().get_to_after_key_up(),
                                       time_stamp_delay,
                                       *output_event_queue);

                  post_from_mandatory_modifiers_key_down(front_input_event,
                                                         *current_manipulated_original_event,
                                                         time_stamp_delay,
                                                         *output_event_queue);
                }
              }

              break;

            case event_type::single:
              break;
          }

          // to_if_held_down_

          if (to_if_held_down_) {
            to_if_held_down_->setup(front_input_event,
                                    current_manipulated_original_event,
                                    output_event_queue,
                                    parameters_.get_basic_to_if_held_down_threshold_milliseconds());
          }

          // to_delayed_action_

          if (to_delayed_action_) {
            to_delayed_action_->setup(front_input_event,
                                      current_manipulated_original_event->get_from_mandatory_modifiers(),
                                      output_event_queue,
                                      parameters_.get_basic_to_delayed_action_delay_milliseconds());
          }

          // increase_time_stamp_delay

          if (time_stamp_delay > 0) {
            output_event_queue->increase_time_stamp_delay(time_stamp_delay - 1);
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

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::queued_event& front_input_event,
                                                                          event_queue& output_event_queue) {
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
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

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());

    if (to_if_held_down_) {
      to_if_held_down_->cancel(front_input_event);
    }

    if (to_delayed_action_) {
      to_delayed_action_->cancel(front_input_event);
    }
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    unset_alone_if_needed(front_input_event.get_original_event(),
                          front_input_event.get_event_type());
  }

  virtual void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
    if (to_if_held_down_) {
      to_if_held_down_->manipulator_timer_invoked(timer_id);
    }

    if (to_delayed_action_) {
      to_delayed_action_->manipulator_timer_invoked(timer_id);
    }
  }

  const from_event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<to_event_definition>& get_to(void) const {
    return to_;
  }

  void post_from_mandatory_modifiers_key_up(const event_queue::queued_event& front_input_event,
                                            manipulated_original_event& current_manipulated_original_event,
                                            uint64_t& time_stamp_delay,
                                            event_queue& output_event_queue) const {
    post_lazy_modifier_key_events(front_input_event,
                                  current_manipulated_original_event.get_from_mandatory_modifiers(),
                                  event_type::key_up,
                                  time_stamp_delay,
                                  output_event_queue);

    current_manipulated_original_event.set_from_mandatory_modifiers_restored(false);
  }

  void post_from_mandatory_modifiers_key_down(const event_queue::queued_event& front_input_event,
                                              manipulated_original_event& current_manipulated_original_event,
                                              uint64_t& time_stamp_delay,
                                              event_queue& output_event_queue) const {
    if (!current_manipulated_original_event.get_from_mandatory_modifiers_restored()) {
      current_manipulated_original_event.set_from_mandatory_modifiers_restored(true);

      post_lazy_modifier_key_events(front_input_event,
                                    current_manipulated_original_event.get_from_mandatory_modifiers(),
                                    event_type::key_down,
                                    time_stamp_delay,
                                    output_event_queue);
    }
  }

  void post_events_at_key_down(const event_queue::queued_event& front_input_event,
                               std::vector<to_event_definition> to_events,
                               manipulated_original_event& current_manipulated_original_event,
                               uint64_t& time_stamp_delay,
                               event_queue& output_event_queue) const {
    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      if (auto event = it->get_event_definition().to_event()) {
        // to_modifier down, to_key down, to_key up, to_modifier up

        auto to_modifier_events = it->make_modifier_events();

        bool is_modifier_key_event = false;
        if (auto key_code = event->get_key_code()) {
          if (types::make_modifier_flag(*key_code) != boost::none) {
            is_modifier_key_event = true;
          }
        }

        {
          // Unset lazy if event is modifier key event in order to keep modifier keys order.

          bool lazy = !is_modifier_key_event || it->get_lazy();
          for (const auto& e : to_modifier_events) {
            auto t = front_input_event.get_event_time_stamp();
            t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

            output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                  t,
                                                  e,
                                                  event_type::key_down,
                                                  front_input_event.get_original_event(),
                                                  lazy);
          }
        }

        // Post key_down event

        {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        }

        // Post key_up event

        if (it != std::end(to_events) - 1 || !it->get_repeat()) {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        } else {
          current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                       *event,
                                                                                       event_type::key_up,
                                                                                       front_input_event.get_original_event(),
                                                                                       it->get_lazy());
        }

        {
          for (const auto& e : to_modifier_events) {
            if (it == std::end(to_events) - 1 && is_modifier_key_event) {
              current_manipulated_original_event.get_events_at_key_up().emplace_back_event(front_input_event.get_device_id(),
                                                                                           e,
                                                                                           event_type::key_up,
                                                                                           front_input_event.get_original_event(),
                                                                                           true);
            } else {
              auto t = front_input_event.get_event_time_stamp();
              t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

              output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                    t,
                                                    e,
                                                    event_type::key_up,
                                                    front_input_event.get_original_event(),
                                                    true);
            }
          }
        }
      }
    }
  }

  void post_events_at_key_up(const event_queue::queued_event& front_input_event,
                             manipulated_original_event& current_manipulated_original_event,
                             uint64_t& time_stamp_delay,
                             event_queue& output_event_queue) const {
    for (const auto& e : current_manipulated_original_event.get_events_at_key_up().get_events()) {
      auto t = front_input_event.get_event_time_stamp();
      t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);
      output_event_queue.push_back_event(e.make_queued_event(t));
    }
    current_manipulated_original_event.get_events_at_key_up().clear_events();
  }

private:
  bool all_from_events_found(const std::unordered_set<manipulated_original_event::from_event, manipulated_original_event::from_event_hash>& from_events) const {
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

  bool is_last_to_event_modifier_key_event(const std::vector<to_event_definition>& to_events) const {
    if (to_events.empty()) {
      return false;
    }

    if (auto event = to_events.back().get_event_definition().to_event()) {
      if (auto key_code = event->get_key_code()) {
        if (types::make_modifier_flag(*key_code) != boost::none) {
          return true;
        }
      }
    }

    return false;
  }

  void post_lazy_modifier_key_events(const event_queue::queued_event& front_input_event,
                                     const std::unordered_set<modifier_flag>& modifiers,
                                     event_type event_type,
                                     uint64_t& time_stamp_delay,
                                     event_queue& output_event_queue) const {
    for (const auto& m : modifiers) {
      if (auto key_code = types::make_key_code(m)) {
        auto t = front_input_event.get_event_time_stamp();
        t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

        event_queue::queued_event event(front_input_event.get_device_id(),
                                        t,
                                        event_queue::queued_event::event(*key_code),
                                        event_type,
                                        front_input_event.get_original_event(),
                                        true);
        output_event_queue.push_back_event(event);
      }
    }
  }

  void post_extra_to_events(const event_queue::queued_event& front_input_event,
                            const std::vector<to_event_definition>& to_events,
                            uint64_t& time_stamp_delay,
                            event_queue& output_event_queue) const {
    for (auto it = std::begin(to_events); it != std::end(to_events); std::advance(it, 1)) {
      auto& to = *it;
      if (auto event = to.get_event_definition().to_event()) {
        auto to_modifier_events = to.make_modifier_events();

        // Post modifier events

        for (const auto& e : to_modifier_events) {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                e,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                true);
        }

        // Post key_down event

        {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_down,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        }

        // Post key_up event

        {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                *event,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                it->get_lazy());
        }

        // Post modifier events

        for (const auto& e : to_modifier_events) {
          auto t = front_input_event.get_event_time_stamp();
          t.set_time_stamp(t.get_time_stamp() + time_stamp_delay++);

          output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                t,
                                                e,
                                                event_type::key_up,
                                                front_input_event.get_original_event(),
                                                true);
        }
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

  core_configuration::profile::complex_modifications::parameters parameters_;

  from_event_definition from_;
  std::vector<to_event_definition> to_;
  std::vector<to_event_definition> to_after_key_up_;
  std::vector<to_event_definition> to_if_alone_;
  std::unique_ptr<to_if_held_down> to_if_held_down_;
  std::unique_ptr<to_delayed_action> to_delayed_action_;

  std::vector<std::shared_ptr<manipulated_original_event>> manipulated_original_events_;
};

inline void from_json(const nlohmann::json& json, basic::from_event_definition::simultaneous_options::key_order& value) {
  auto s = json.get<std::string>();

  if (s == "insensitive") {
    value = basic::from_event_definition::simultaneous_options::key_order::insensitive;
  } else if (s == "strict") {
    value = basic::from_event_definition::simultaneous_options::key_order::strict;
  } else if (s == "strict_inverse") {
    value = basic::from_event_definition::simultaneous_options::key_order::strict_inverse;
  } else {
    logger::get_logger().error("complex_modifications json error: Unknown simultaneous_options::key_order: {0}", json.dump());
    value = basic::from_event_definition::simultaneous_options::key_order::insensitive;
  }
}
} // namespace details
} // namespace manipulator
} // namespace krbn
