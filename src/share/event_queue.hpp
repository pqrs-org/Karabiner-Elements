#pragma once

#include "boost_defs.hpp"

#include "json_utility.hpp"
#include "manipulator_environment.hpp"
#include "modifier_flag_manager.hpp"
#include "pointing_button_manager.hpp"
#include "stream_utility.hpp"
#include "types.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace krbn {
class event_queue final {
public:
  class queued_event final {
  public:
    class event {
    public:
      enum class type {
        none,
        key_code,
        consumer_key_code,
        pointing_button,
        pointing_x,
        pointing_y,
        pointing_vertical_wheel,
        pointing_horizontal_wheel,
        // virtual events
        shell_command,
        select_input_source,
        set_variable,
        mouse_key,
        // virtual events (passive)
        device_keys_and_pointing_buttons_are_released,
        device_ungrabbed,
        caps_lock_state_changed,
        event_from_ignored_device,
        pointing_device_event_from_event_tap,
        frontmost_application_changed,
        input_source_changed,
        keyboard_type_changed,
      };

      event(void) : type_(type::none),
                    value_(boost::blank()) {
      }

      event(const nlohmann::json& json) : type_(type::none),
                                          value_(boost::blank()) {
        if (auto v = json_utility::find_optional<std::string>(json, "type")) {
          type_ = to_type(*v);
        }

        switch (type_) {
          case type::none:
            break;

          case type::key_code:
            if (auto s = json_utility::find_optional<std::string>(json, "key_code")) {
              if (auto v = types::make_key_code(*s)) {
                value_ = *v;
              }
            }
            break;

          case type::consumer_key_code:
            if (auto s = json_utility::find_optional<std::string>(json, "consumer_key_code")) {
              if (auto v = types::make_consumer_key_code(*s)) {
                value_ = *v;
              }
            }
            break;

          case type::pointing_button:
            if (auto s = json_utility::find_optional<std::string>(json, "pointing_button")) {
              if (auto v = types::make_pointing_button(*s)) {
                value_ = *v;
              }
            }
            break;

          case type::pointing_x:
          case type::pointing_y:
          case type::pointing_vertical_wheel:
          case type::pointing_horizontal_wheel:
          case type::caps_lock_state_changed:
            if (auto v = json_utility::find_optional<int>(json, "integer_value")) {
              value_ = *v;
            }
            break;

          case type::shell_command:
            if (auto v = json_utility::find_optional<std::string>(json, "shell_command")) {
              value_ = *v;
            }
            break;

          case type::select_input_source:
            if (auto v = json_utility::find_array(json, "input_source_selectors")) {
              std::vector<input_source_selector> input_source_selectors;
              for (const auto& j : *v) {
                input_source_selectors.emplace_back(j);
              }
              value_ = input_source_selectors;
            }
            break;

          case type::set_variable:
            if (auto o = json_utility::find_object(json, "set_variable")) {
              std::pair<std::string, int> pair;
              if (auto v = json_utility::find_optional<std::string>(*o, "name")) {
                pair.first = *v;
              }
              if (auto v = json_utility::find_optional<int>(*o, "value")) {
                pair.second = *v;
              }
              value_ = pair;
            }
            break;

          case type::mouse_key:
            if (auto v = json_utility::find_json(json, "mouse_key")) {
              value_ = mouse_key(*v);
            }
            break;

          case type::frontmost_application_changed:
            if (auto v = json_utility::find_json(json, "frontmost_application")) {
              value_ = manipulator_environment::frontmost_application(*v);
            }
            break;

          case type::input_source_changed:
            if (auto v = json_utility::find_json(json, "input_source_identifiers")) {
              value_ = input_source_identifiers(*v);
            }
            break;

          case type::keyboard_type_changed:
            if (auto v = json_utility::find_optional<std::string>(json, "keyboard_type")) {
              value_ = *v;
            }
            break;

          case type::device_keys_and_pointing_buttons_are_released:
          case type::device_ungrabbed:
          case type::event_from_ignored_device:
          case type::pointing_device_event_from_event_tap:
            break;
        }
      }

      nlohmann::json to_json(void) const {
        nlohmann::json json;
        json["type"] = to_c_string(type_);

        switch (type_) {
          case type::none:
            break;

          case type::key_code:
            if (auto v = get_key_code()) {
              if (auto s = types::make_key_code_name(*v)) {
                json["key_code"] = *s;
              }
            }
            break;

          case type::consumer_key_code:
            if (auto v = get_consumer_key_code()) {
              if (auto s = types::make_consumer_key_code_name(*v)) {
                json["consumer_key_code"] = *s;
              }
            }
            break;

          case type::pointing_button:
            if (auto v = get_pointing_button()) {
              if (auto s = types::make_pointing_button_name(*v)) {
                json["pointing_button"] = *s;
              }
            }
            break;

          case type::pointing_x:
          case type::pointing_y:
          case type::pointing_vertical_wheel:
          case type::pointing_horizontal_wheel:
          case type::caps_lock_state_changed:
            if (auto v = get_integer_value()) {
              json["integer_value"] = *v;
            }
            break;

          case type::shell_command:
            if (auto v = get_shell_command()) {
              json["shell_command"] = *v;
            }
            break;

          case type::select_input_source:
            if (auto v = get_input_source_selectors()) {
              json["input_source_selectors"] = *v;
            }
            break;

          case type::set_variable:
            if (auto v = get_set_variable()) {
              json["set_variable"]["name"] = v->first;
              json["set_variable"]["value"] = v->second;
            }
            break;

          case type::mouse_key:
            if (auto v = get_mouse_key()) {
              json["mouse_key"] = v->to_json();
            }
            break;

          case type::frontmost_application_changed:
            if (auto v = get_frontmost_application()) {
              json["frontmost_application"] = v->to_json();
            }
            break;

          case type::input_source_changed:
            if (auto v = get_input_source_identifiers()) {
              json["input_source_identifiers"] = v->to_json();
            }
            break;

          case type::keyboard_type_changed:
            if (auto v = get_keyboard_type()) {
              json["keyboard_type"] = *v;
            }
            break;

          case type::device_keys_and_pointing_buttons_are_released:
          case type::device_ungrabbed:
          case type::event_from_ignored_device:
          case type::pointing_device_event_from_event_tap:
            break;
        }

        return json;
      }

      event(key_code key_code) : type_(type::key_code),
                                 value_(key_code) {
      }

      event(consumer_key_code consumer_key_code) : type_(type::consumer_key_code),
                                                   value_(consumer_key_code) {
      }

      event(pointing_button pointing_button) : type_(type::pointing_button),
                                               value_(pointing_button) {
      }

      event(type type,
            int64_t integer_value) : type_(type),
                                     value_(integer_value) {
      }

      static event make_shell_command_event(const std::string& shell_command) {
        event e;
        e.type_ = type::shell_command;
        e.value_ = shell_command;
        return e;
      }

      static event make_select_input_source_event(const std::vector<input_source_selector>& input_source_selector) {
        event e;
        e.type_ = type::select_input_source;
        e.value_ = input_source_selector;
        return e;
      }

      static event make_set_variable_event(const std::pair<std::string, int>& pair) {
        event e;
        e.type_ = type::set_variable;
        e.value_ = pair;
        return e;
      }

      static event make_mouse_key_event(const mouse_key& mouse_key) {
        event e;
        e.type_ = type::mouse_key;
        e.value_ = mouse_key;
        return e;
      }

      static event make_device_keys_and_pointing_buttons_are_released_event(void) {
        return make_virtual_event(type::device_keys_and_pointing_buttons_are_released);
      }

      static event make_device_ungrabbed_event(void) {
        return make_virtual_event(type::device_ungrabbed);
      }

      static event make_event_from_ignored_device_event(void) {
        return make_virtual_event(type::event_from_ignored_device);
      }

      static event make_pointing_device_event_from_event_tap_event(void) {
        return make_virtual_event(type::pointing_device_event_from_event_tap);
      }

      static event make_frontmost_application_changed_event(const std::string& bundle_identifier,
                                                            const std::string& file_path) {
        event e;
        e.type_ = type::frontmost_application_changed;
        e.value_ = manipulator_environment::frontmost_application(bundle_identifier,
                                                                  file_path);
        return e;
      }

      static event make_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
        event e;
        e.type_ = type::input_source_changed;
        e.value_ = input_source_identifiers;
        return e;
      }

      static event make_keyboard_type_changed_event(const std::string& keyboard_type) {
        event e;
        e.type_ = type::keyboard_type_changed;
        e.value_ = keyboard_type;
        return e;
      }

      type get_type(void) const {
        return type_;
      }

      boost::optional<key_code> get_key_code(void) const {
        try {
          if (type_ == type::key_code) {
            return boost::get<key_code>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<consumer_key_code> get_consumer_key_code(void) const {
        try {
          if (type_ == type::consumer_key_code) {
            return boost::get<consumer_key_code>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<pointing_button> get_pointing_button(void) const {
        try {
          if (type_ == type::pointing_button) {
            return boost::get<pointing_button>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<int64_t> get_integer_value(void) const {
        try {
          if (type_ == type::pointing_x ||
              type_ == type::pointing_y ||
              type_ == type::pointing_vertical_wheel ||
              type_ == type::pointing_horizontal_wheel ||
              type_ == type::caps_lock_state_changed) {
            return boost::get<int64_t>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<std::string> get_shell_command(void) const {
        try {
          if (type_ == type::shell_command) {
            return boost::get<std::string>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<std::vector<input_source_selector>> get_input_source_selectors(void) const {
        try {
          if (type_ == type::select_input_source) {
            return boost::get<std::vector<input_source_selector>>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<std::pair<std::string, int>> get_set_variable(void) const {
        try {
          if (type_ == type::set_variable) {
            return boost::get<std::pair<std::string, int>>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<mouse_key> get_mouse_key(void) const {
        try {
          if (type_ == type::mouse_key) {
            return boost::get<mouse_key>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<manipulator_environment::frontmost_application> get_frontmost_application(void) const {
        try {
          if (type_ == type::frontmost_application_changed) {
            return boost::get<manipulator_environment::frontmost_application>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<input_source_identifiers> get_input_source_identifiers(void) const {
        try {
          if (type_ == type::input_source_changed) {
            return boost::get<input_source_identifiers>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      boost::optional<std::string> get_keyboard_type(void) const {
        try {
          if (type_ == type::keyboard_type_changed) {
            return boost::get<std::string>(value_);
          }
        } catch (boost::bad_get&) {
        }
        return boost::none;
      }

      bool operator==(const event& other) const {
        return get_type() == other.get_type() &&
               value_ == other.value_;
      }

    private:
      static event make_virtual_event(type type) {
        event e;
        e.type_ = type;
        e.value_ = boost::blank();
        return e;
      }

      static const char* to_c_string(type t) {
#define TO_C_STRING(TYPE) \
  case type::TYPE:        \
    return #TYPE;

        switch (t) {
          TO_C_STRING(none);
          TO_C_STRING(key_code);
          TO_C_STRING(consumer_key_code);
          TO_C_STRING(pointing_button);
          TO_C_STRING(pointing_x);
          TO_C_STRING(pointing_y);
          TO_C_STRING(pointing_vertical_wheel);
          TO_C_STRING(pointing_horizontal_wheel);
          TO_C_STRING(shell_command);
          TO_C_STRING(select_input_source);
          TO_C_STRING(set_variable);
          TO_C_STRING(mouse_key);
          TO_C_STRING(device_keys_and_pointing_buttons_are_released);
          TO_C_STRING(device_ungrabbed);
          TO_C_STRING(caps_lock_state_changed);
          TO_C_STRING(event_from_ignored_device);
          TO_C_STRING(pointing_device_event_from_event_tap);
          TO_C_STRING(frontmost_application_changed);
          TO_C_STRING(input_source_changed);
          TO_C_STRING(keyboard_type_changed);
        }

#undef TO_C_STRING

        return nullptr;
      }

      static type to_type(const std::string& t) {
#define TO_TYPE(TYPE)    \
  {                      \
    if (t == #TYPE) {    \
      return type::TYPE; \
    }                    \
  }

        TO_TYPE(key_code);
        TO_TYPE(consumer_key_code);
        TO_TYPE(pointing_button);
        TO_TYPE(pointing_x);
        TO_TYPE(pointing_y);
        TO_TYPE(pointing_vertical_wheel);
        TO_TYPE(pointing_horizontal_wheel);
        TO_TYPE(shell_command);
        TO_TYPE(select_input_source);
        TO_TYPE(set_variable);
        TO_TYPE(mouse_key);
        TO_TYPE(device_keys_and_pointing_buttons_are_released);
        TO_TYPE(device_ungrabbed);
        TO_TYPE(caps_lock_state_changed);
        TO_TYPE(event_from_ignored_device);
        TO_TYPE(pointing_device_event_from_event_tap);
        TO_TYPE(frontmost_application_changed);
        TO_TYPE(input_source_changed);
        TO_TYPE(keyboard_type_changed);

#undef TO_TYPE

        return type::none;
      }

      type type_;

      boost::variant<key_code, // For type::key_code
                     consumer_key_code, // For type::consumer_key_code
                     pointing_button, // For type::pointing_button
                     int64_t, // For type::pointing_x, type::pointing_y, type::pointing_vertical_wheel, type::pointing_horizontal_wheel
                     std::string, // For shell_command, keyboard_type_changed
                     std::vector<input_source_selector>, // For select_input_source
                     std::pair<std::string, int>, // For set_variable
                     mouse_key, // For mouse_key
                     manipulator_environment::frontmost_application, // For frontmost_application_changed
                     input_source_identifiers, // For input_source_changed
                     boost::blank> // For virtual events
          value_;
    };

    queued_event(device_id device_id,
                 uint64_t time_stamp,
                 const class event& event,
                 event_type event_type,
                 const class event& original_event,
                 bool lazy = false) : device_id_(device_id),
                                      time_stamp_(time_stamp),
                                      valid_(true),
                                      lazy_(lazy),
                                      event_(event),
                                      event_type_(event_type),
                                      original_event_(original_event) {
    }

    queued_event(const nlohmann::json& json) : device_id_(device_id::zero),
                                               time_stamp_(0),
                                               valid_(true),
                                               lazy_(false),
                                               event_type_(event_type::key_down) {
      if (json.is_object()) {
        {
          auto it = json.find("device_id");
          if (it != std::end(json)) {
            device_id_ = device_id(it->get<int>());
          }
        }
        {
          auto it = json.find("time_stamp");
          if (it != std::end(json)) {
            time_stamp_ = *it;
          }
        }
        {
          auto it = json.find("valid");
          if (it != std::end(json)) {
            valid_ = *it;
          }
        }
        {
          auto it = json.find("lazy");
          if (it != std::end(json)) {
            lazy_ = *it;
          }
        }
        {
          auto it = json.find("event");
          if (it != std::end(json)) {
            event_ = *it;
          }
        }
        {
          auto it = json.find("event_type");
          if (it != std::end(json)) {
            event_type_ = *it;
          }
        }
        {
          auto it = json.find("original_event");
          if (it != std::end(json)) {
            original_event_ = *it;
          }
        }
      }
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"device_id", static_cast<uint32_t>(device_id_)},
          {"time_stamp", time_stamp_},
          {"valid", valid_},
          {"lazy", lazy_},
          {"event", event_},
          {"event_type", event_type_},
          {"original_event", original_event_},
      });
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    uint64_t get_time_stamp(void) const {
      return time_stamp_;
    }

    bool get_valid(void) const {
      return valid_;
    }
    void set_valid(bool value) {
      valid_ = value;
    }

    bool get_lazy(void) const {
      return lazy_;
    }
    void set_lazy(bool value) {
      lazy_ = value;
    }

    const event& get_event(void) const {
      return event_;
    }

    event_type get_event_type(void) const {
      return event_type_;
    }

    const event& get_original_event(void) const {
      return original_event_;
    }

    bool operator==(const queued_event& other) const {
      return get_device_id() == other.get_device_id() &&
             get_time_stamp() == other.get_time_stamp() &&
             get_valid() == other.get_valid() &&
             get_lazy() == other.get_lazy() &&
             get_event() == other.get_event() &&
             get_event_type() == other.get_event_type() &&
             get_original_event() == other.get_original_event();
    }

  private:
    device_id device_id_;
    uint64_t time_stamp_;
    bool valid_;
    bool lazy_;
    event event_;
    event_type event_type_;
    event original_event_;
  };

  event_queue(const event_queue&) = delete;

  event_queue(void) : time_stamp_delay_(0) {
  }

  // from physical device
  bool emplace_back_event(device_id device_id,
                          uint64_t time_stamp,
                          hid_usage_page usage_page,
                          hid_usage usage,
                          int64_t integer_value) {
    if (auto key_code = types::make_key_code(usage_page, usage)) {
      queued_event::event event(*key_code);
      emplace_back_event(device_id,
                         time_stamp,
                         event,
                         integer_value ? event_type::key_down : event_type::key_up,
                         event);
      return true;
    }

    if (auto consumer_key_code = types::make_consumer_key_code(usage_page, usage)) {
      queued_event::event event(*consumer_key_code);
      emplace_back_event(device_id,
                         time_stamp,
                         event,
                         integer_value ? event_type::key_down : event_type::key_up,
                         event);
      return true;
    }

    if (auto pointing_button = types::make_pointing_button(usage_page, usage)) {
      queued_event::event event(*pointing_button);
      emplace_back_event(device_id,
                         time_stamp,
                         event,
                         integer_value ? event_type::key_down : event_type::key_up,
                         event);
      return true;
    }

    switch (usage_page) {
      case hid_usage_page::generic_desktop:
        switch (usage) {
          case hid_usage::gd_x: {
            queued_event::event event(queued_event::event::type::pointing_x, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::single,
                               event);
            return true;
          }

          case hid_usage::gd_y: {
            queued_event::event event(queued_event::event::type::pointing_y, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::single,
                               event);
            return true;
          }

          case hid_usage::gd_wheel: {
            queued_event::event event(queued_event::event::type::pointing_vertical_wheel, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::single,
                               event);
            return true;
          }

          default:
            break;
        }
        break;

      case hid_usage_page::consumer:
        switch (usage) {
          case hid_usage::csmr_acpan: {
            queued_event::event event(queued_event::event::type::pointing_horizontal_wheel, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::single,
                               event);
            return true;
          }

          default:
            break;
        }
        break;

      default:
        break;
    }

    return false;
  }

  void emplace_back_event(device_id device_id,
                          uint64_t time_stamp,
                          const queued_event::event& event,
                          event_type event_type,
                          const queued_event::event& original_event,
                          bool lazy = false) {
    time_stamp += time_stamp_delay_;

    events_.emplace_back(device_id,
                         time_stamp,
                         event,
                         event_type,
                         original_event,
                         lazy);

    sort_events();

    // Update modifier_flag_manager

    if (auto key_code = event.get_key_code()) {
      if (auto modifier_flag = types::make_modifier_flag(*key_code)) {
        auto type = (event_type == event_type::key_down ? modifier_flag_manager::active_modifier_flag::type::increase
                                                        : modifier_flag_manager::active_modifier_flag::type::decrease);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         *modifier_flag,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }
    }

    if (event.get_type() == queued_event::event::type::caps_lock_state_changed) {
      if (auto integer_value = event.get_integer_value()) {
        auto type = (*integer_value ? modifier_flag_manager::active_modifier_flag::type::increase_lock
                                    : modifier_flag_manager::active_modifier_flag::type::decrease_lock);
        modifier_flag_manager::active_modifier_flag active_modifier_flag(type,
                                                                         modifier_flag::caps_lock,
                                                                         device_id);
        modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
      }
    }

    // Update pointing_button_manager

    if (auto pointing_button = event.get_pointing_button()) {
      if (*pointing_button != pointing_button::zero) {
        auto type = (event_type == event_type::key_down ? pointing_button_manager::active_pointing_button::type::increase
                                                        : pointing_button_manager::active_pointing_button::type::decrease);
        pointing_button_manager::active_pointing_button active_pointing_button(type,
                                                                               *pointing_button,
                                                                               device_id);
        pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
      }
    }

    // Update manipulator_environment
    if (auto frontmost_application = event.get_frontmost_application()) {
      manipulator_environment_.set_frontmost_application(*frontmost_application);
    }
    if (auto input_source_identifiers = event.get_input_source_identifiers()) {
      manipulator_environment_.set_input_source_identifiers(*input_source_identifiers);
    }
    if (event_type == event_type::key_down) {
      if (auto set_variable = event.get_set_variable()) {
        manipulator_environment_.set_variable(set_variable->first,
                                              set_variable->second);
      }
    }
    if (auto keyboard_type = event.get_keyboard_type()) {
      manipulator_environment_.set_keyboard_type(*keyboard_type);
    }
  }

  void push_back_event(const queued_event& queued_event) {
    emplace_back_event(queued_event.get_device_id(),
                       queued_event.get_time_stamp(),
                       queued_event.get_event(),
                       queued_event.get_event_type(),
                       queued_event.get_original_event(),
                       queued_event.get_lazy());
  }

  void clear_events(void) {
    events_.clear();
    time_stamp_delay_ = 0;
  }

  queued_event& get_front_event(void) {
    return events_.front();
  }

  void erase_front_event(void) {
    events_.erase(std::begin(events_));
    if (events_.empty()) {
      time_stamp_delay_ = 0;
    }
  }

  bool empty(void) const {
    return events_.empty();
  }

  const std::vector<queued_event>& get_events(void) const {
    return events_;
  }

  const modifier_flag_manager& get_modifier_flag_manager(void) const {
    return modifier_flag_manager_;
  }

  void erase_all_active_modifier_flags_except_lock(device_id device_id) {
    modifier_flag_manager_.erase_all_active_modifier_flags_except_lock(device_id);
  }

  void erase_all_active_modifier_flags(device_id device_id) {
    modifier_flag_manager_.erase_all_active_modifier_flags(device_id);
  }

  const pointing_button_manager& get_pointing_button_manager(void) const {
    return pointing_button_manager_;
  }

  void erase_all_active_pointing_buttons_except_lock(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons_except_lock(device_id);
  }

  void erase_all_active_pointing_buttons(device_id device_id) {
    pointing_button_manager_.erase_all_active_pointing_buttons(device_id);
  }

  const manipulator_environment& get_manipulator_environment(void) const {
    return manipulator_environment_;
  }

  void enable_manipulator_environment_json_output(const std::string& file_path) {
    manipulator_environment_.enable_json_output(file_path);
  }

  void disable_manipulator_environment_json_output(void) {
    manipulator_environment_.disable_json_output();
  }

  uint64_t get_time_stamp_delay(void) const {
    return time_stamp_delay_;
  }

  void increase_time_stamp_delay(uint64_t value) {
    time_stamp_delay_ += value;
  }

  static bool needs_swap(const queued_event& v1, const queued_event& v2) {
    // Some devices are send modifier flag and key at the same HID report.
    // For example, a key sends control+up-arrow by this reports.
    //
    //   modifiers: 0x0
    //   keys: 0x0 0x0 0x0 0x0 0x0 0x0
    //
    //   modifiers: 0x1
    //   keys: 0x52 0x0 0x0 0x0 0x0 0x0
    //
    // In this case, macOS does not guarantee the value event order to be modifier first.
    // At least macOS 10.12 or prior sends the up-arrow event first.
    //
    //   ----------------------------------------
    //   Example of hid value events in a single queue at control+up-arrow
    //
    //   1. up-arrow keydown
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:1
    //
    //   2. control keydown
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:1
    //
    //   3. up-arrow keyup
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:0
    //
    //   4. control keyup
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:0
    //   ----------------------------------------
    //
    // These events will not be interpreted as intended in this order.
    // Thus, we have to reorder the events.

    if (v1.get_time_stamp() == v2.get_time_stamp()) {
      auto key_code1 = v1.get_event().get_key_code();
      auto key_code2 = v2.get_event().get_key_code();

      if (key_code1 && key_code2) {
        auto modifier_flag1 = types::make_modifier_flag(*key_code1);
        auto modifier_flag2 = types::make_modifier_flag(*key_code2);

        // If either modifier_flag1 or modifier_flag2 is modifier, reorder it before.

        if (!modifier_flag1 && modifier_flag2) {
          // v2 is modifier_flag
          if (v2.get_event_type() == event_type::key_up) {
            return false;
          } else {
            // reorder to v2,v1 if v2 is pressed.
            return true;
          }
        }

        if (modifier_flag1 && !modifier_flag2) {
          // v1 is modifier_flag
          if (v1.get_event_type() == event_type::key_up) {
            // reorder to v2,v1 if v1 is released.
            return true;
          } else {
            return false;
          }
        }
      }
    }

    return false;
  }

private:
  void sort_events(void) {
    for (size_t i = 0; i < events_.size() - 1;) {
      if (needs_swap(events_[i], events_[i + 1])) {
        std::swap(events_[i], events_[i + 1]);
        if (i > 0) {
          --i;
        }
        continue;
      }
      ++i;
    }
  }

  std::vector<queued_event> events_;
  modifier_flag_manager modifier_flag_manager_;
  pointing_button_manager pointing_button_manager_;
  manipulator_environment manipulator_environment_;
  uint64_t time_stamp_delay_;
}; // namespace krbn

// For unit tests

inline std::ostream& operator<<(std::ostream& stream, const event_queue::queued_event::event::type& value) {
  return stream_utility::output_enum(stream, value);
}

inline std::ostream& operator<<(std::ostream& stream, const event_queue::queued_event::event& event) {
  stream << "{"
         << "\"type\":";
  stream_utility::output_enum(stream, event.get_type());

  if (auto key_code = event.get_key_code()) {
    stream << ",\"key_code\":" << *key_code;
  }

  if (auto pointing_button = event.get_pointing_button()) {
    stream << ",\"pointing_button\":" << *pointing_button;
  }

  if (auto integer_value = event.get_integer_value()) {
    stream << ",\"integer_value\":" << *integer_value;
  }

  stream << "}";

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const event_queue::queued_event& value) {
  stream << std::endl
         << "{"
         << "\"device_id\":" << value.get_device_id()
         << ",\"time_stamp\":" << value.get_time_stamp()
         << ",\"valid\":" << value.get_valid()
         << ",\"lazy\":" << value.get_lazy()
         << ",\"event\":" << value.get_event()
         << ",\"event_type\":" << value.get_event_type()
         << ",\"original_event\":" << value.get_original_event()
         << "}";
  return stream;
}

inline void to_json(nlohmann::json& json, const event_queue::queued_event::event& value) {
  json = value.to_json();
}

inline void to_json(nlohmann::json& json, const event_queue::queued_event& value) {
  json = value.to_json();
}
} // namespace krbn
