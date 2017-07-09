#pragma once

#include "boost_defs.hpp"

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
        key_code,
        pointing_button,
        pointing_x,
        pointing_y,
        pointing_vertical_wheel,
        pointing_horizontal_wheel,
        // virtual events
        device_keys_are_released,
        device_pointing_buttons_are_released,
        device_ungrabbed,
        caps_lock_state_changed,
        event_from_ignored_device,
        frontmost_application_changed,
      };

      event(key_code key_code) : type_(type::key_code),
                                 value_(key_code) {
      }

      event(pointing_button pointing_button) : type_(type::pointing_button),
                                               value_(pointing_button) {
      }

      event(type type,
            int64_t integer_value) : type_(type),
                                     value_(integer_value) {
      }

      static event make_event_from_ignored_device(type original_type,
                                                  boost::optional<int64_t> original_integer_value) {
        event e;
        e.type_ = type::event_from_ignored_device;
        e.value_ = original_value(original_type,
                                  (original_integer_value ? *original_integer_value : 0));
        return e;
      }

      static event make_frontmost_application_changed(const std::string& bundle_identifier,
                                                      const std::string& file_path) {
        event e;
        e.type_ = type::frontmost_application_changed;
        e.value_ = frontmost_application(bundle_identifier, file_path);
        return e;
      }

      type get_type(void) const {
        return type_;
      }

      boost::optional<key_code> get_key_code(void) const {
        if (type_ == type::key_code) {
          return boost::get<key_code>(value_);
        }
        return boost::none;
      }

      boost::optional<pointing_button> get_pointing_button(void) const {
        if (type_ == type::pointing_button) {
          return boost::get<pointing_button>(value_);
        }
        return boost::none;
      }

      boost::optional<int64_t> get_integer_value(void) const {
        if (type_ == type::pointing_x ||
            type_ == type::pointing_y ||
            type_ == type::pointing_vertical_wheel ||
            type_ == type::pointing_horizontal_wheel ||
            type_ == type::caps_lock_state_changed) {
          return boost::get<int64_t>(value_);
        }
        return boost::none;
      }

      boost::optional<type> get_original_type(void) const {
        if (type_ == type::event_from_ignored_device) {
          const auto& v = boost::get<original_value>(value_);
          return v.get_type();
        }
        return boost::none;
      }

      boost::optional<int64_t> get_original_integer_value(void) const {
        if (type_ == type::event_from_ignored_device) {
          const auto& v = boost::get<original_value>(value_);
          return v.get_integer_value();
        }
        return boost::none;
      }

      boost::optional<std::string> get_frontmost_application_bundle_identifier(void) const {
        if (type_ == type::frontmost_application_changed) {
          const auto& v = boost::get<frontmost_application>(value_);
          return v.get_bundle_identifier();
        }
        return boost::none;
      }

      boost::optional<std::string> get_frontmost_application_file_path(void) const {
        if (type_ == type::frontmost_application_changed) {
          const auto& v = boost::get<frontmost_application>(value_);
          return v.get_file_path();
        }
        return boost::none;
      }

      bool operator==(const event& other) const {
        return get_type() == other.get_type() &&
               value_ == other.value_;
      }

    private:
      class original_value final {
      public:
        original_value(type type,
                       int64_t integer_value) : type_(type),
                                                integer_value_(integer_value) {
        }

        type get_type(void) const {
          return type_;
        }

        int64_t get_integer_value(void) const {
          return integer_value_;
        }

        bool operator==(const original_value& other) const {
          return type_ == other.type_ &&
                 integer_value_ == other.integer_value_;
        }

      private:
        type type_;
        int64_t integer_value_;
      };

      class frontmost_application final {
      public:
        frontmost_application(const std::string& bundle_identifier,
                              const std::string& file_path) : bundle_identifier_(bundle_identifier),
                                                              file_path_(file_path) {
        }

        const std::string& get_bundle_identifier(void) const {
          return bundle_identifier_;
        }

        const std::string& get_file_path(void) const {
          return file_path_;
        }

        bool operator==(const frontmost_application& other) const {
          return bundle_identifier_ == other.bundle_identifier_ &&
                 file_path_ == other.file_path_;
        }

      private:
        std::string bundle_identifier_;
        std::string file_path_;
      };

      event(void) {
      }

      type type_;

      boost::variant<key_code,        // For type::key_code
                     pointing_button, // For type::pointing_button
                     int64_t,         // For type::pointing_x, type::pointing_y, type::pointing_vertical_wheel, type::pointing_horizontal_wheel and virtual events
                     original_value,  // For type::event_from_ignored_device
                     frontmost_application>
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
    if (auto key_code = types::get_key_code(usage_page, usage)) {
      queued_event::event event(*key_code);
      emplace_back_event(device_id,
                         time_stamp,
                         event,
                         integer_value ? event_type::key_down : event_type::key_up,
                         event);
      return true;
    }

    if (auto pointing_button = types::get_pointing_button(usage_page, usage)) {
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
                               event_type::key_down,
                               event);
            return true;
          }

          case hid_usage::gd_y: {
            queued_event::event event(queued_event::event::type::pointing_y, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::key_down,
                               event);
            return true;
          }

          case hid_usage::gd_wheel: {
            queued_event::event event(queued_event::event::type::pointing_vertical_wheel, integer_value);
            emplace_back_event(device_id,
                               time_stamp,
                               event,
                               event_type::key_down,
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
                               event_type::key_down,
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
      auto modifier_flag = types::get_modifier_flag(*key_code);
      if (modifier_flag != modifier_flag::zero) {
        modifier_flag_manager::active_modifier_flag active_modifier_flag(modifier_flag_manager::active_modifier_flag::type::increase,
                                                                         modifier_flag,
                                                                         device_id);
        if (event_type == event_type::key_down) {
          modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
        } else {
          modifier_flag_manager_.erase_active_modifier_flag(active_modifier_flag);
        }
      }
    }

    if (event.get_type() == queued_event::event::type::caps_lock_state_changed) {
      if (auto integer_value = event.get_integer_value()) {
        modifier_flag_manager::active_modifier_flag active_modifier_flag(modifier_flag_manager::active_modifier_flag::type::increase_lock,
                                                                         modifier_flag::caps_lock,
                                                                         device_id);
        if (*integer_value) {
          modifier_flag_manager_.push_back_active_modifier_flag(active_modifier_flag);
        } else {
          modifier_flag_manager_.erase_all_active_modifier_flags(active_modifier_flag);
        }
      }
    }

    // Update pointing_button_manager

    if (auto pointing_button = event.get_pointing_button()) {
      if (*pointing_button != pointing_button::zero) {
        pointing_button_manager::active_pointing_button active_pointing_button(pointing_button_manager::active_pointing_button::type::increase,
                                                                               *pointing_button,
                                                                               device_id);
        if (event_type == event_type::key_down) {
          pointing_button_manager_.push_back_active_pointing_button(active_pointing_button);
        } else {
          pointing_button_manager_.erase_active_pointing_button(active_pointing_button);
        }
      }
    }

    // Update manipulator_environment
    if (auto bundle_identifier = event.get_frontmost_application_bundle_identifier()) {
      if (auto file_path = event.get_frontmost_application_file_path()) {
        manipulator_environment_.get_frontmost_application().set_bundle_identifier(*bundle_identifier);
        manipulator_environment_.get_frontmost_application().set_file_path(*file_path);
      }
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
        auto modifier_flag1 = types::get_modifier_flag(*key_code1);
        auto modifier_flag2 = types::get_modifier_flag(*key_code2);

        // If either modifier_flag1 or modifier_flag2 is modifier, reorder it before.

        if (modifier_flag1 == modifier_flag::zero &&
            modifier_flag2 != modifier_flag::zero) {
          // v2 is modifier_flag
          if (v2.get_event_type() == event_type::key_up) {
            return false;
          } else {
            // reorder to v2,v1 if v2 is pressed.
            return true;
          }
        }

        if (modifier_flag1 != modifier_flag::zero &&
            modifier_flag2 == modifier_flag::zero) {
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

} // namespace krbn
