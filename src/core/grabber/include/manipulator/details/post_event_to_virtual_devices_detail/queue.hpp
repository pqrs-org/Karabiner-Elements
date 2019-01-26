#pragma once

#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source_selector.hpp>

namespace krbn {
namespace manipulator {
namespace details {
namespace post_event_to_virtual_devices_detail {
class queue final : pqrs::dispatcher::extra::dispatcher_client {
public:
  class event final {
  public:
    enum class type {
      keyboard_input,
      consumer_input,
      apple_vendor_top_case_input,
      apple_vendor_keyboard_input,
      pointing_input,
      shell_command,
      select_input_source,
    };

    event(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& value,
          absolute_time_point time_stamp) : type_(type::keyboard_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner_virtual_hid_device::hid_report::consumer_input& value,
          absolute_time_point time_stamp) : type_(type::consumer_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input& value,
          absolute_time_point time_stamp) : type_(type::apple_vendor_top_case_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input& value,
          absolute_time_point time_stamp) : type_(type::apple_vendor_keyboard_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& value,
          absolute_time_point time_stamp) : type_(type::pointing_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    static event make_shell_command_event(const std::string& shell_command,
                                          absolute_time_point time_stamp) {
      event e;
      e.type_ = type::shell_command;
      e.value_ = shell_command;
      e.time_stamp_ = time_stamp;
      return e;
    }

    static event make_select_input_source_event(const std::vector<input_source_selector>& input_source_selector,
                                                absolute_time_point time_stamp) {
      event e;
      e.type_ = type::select_input_source;
      e.value_ = input_source_selector;
      e.time_stamp_ = time_stamp;
      return e;
    }

    nlohmann::json to_json(void) const {
      nlohmann::json json;
      json["type"] = to_c_string(type_);
      json["time_stamp"] = time_utility::to_milliseconds(time_stamp_ - absolute_time_point(0)).count();

      switch (type_) {
        case type::keyboard_input:
          if (auto v = get_keyboard_input()) {
            json["keyboard_input"]["modifiers"] = v->modifiers;
            json["keyboard_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys, hid_usage_page::keyboard_or_keypad);
          }
          break;

        case type::consumer_input:
          if (auto v = get_consumer_input()) {
            json["consumer_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys, hid_usage_page::consumer);
          }
          break;

        case type::apple_vendor_top_case_input:
          if (auto v = get_apple_vendor_top_case_input()) {
            json["apple_vendor_top_case_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys, hid_usage_page::apple_vendor_top_case);
          }
          break;

        case type::apple_vendor_keyboard_input:
          if (auto v = get_apple_vendor_keyboard_input()) {
            json["apple_vendor_keyboard_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys, hid_usage_page::apple_vendor_keyboard);
          }
          break;

        case type::pointing_input:
          if (auto v = get_pointing_input()) {
            json["pointing_input"]["buttons"] = v->buttons;
            json["pointing_input"]["x"] = static_cast<int>(v->x);
            json["pointing_input"]["y"] = static_cast<int>(v->y);
            json["pointing_input"]["vertical_wheel"] = static_cast<int>(v->vertical_wheel);
            json["pointing_input"]["horizontal_wheel"] = static_cast<int>(v->horizontal_wheel);
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
      }

      return json;
    }

    type get_type(void) const {
      return type_;
    }

    std::optional<pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input> get_keyboard_input(void) const {
      if (type_ == type::keyboard_input) {
        return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner_virtual_hid_device::hid_report::consumer_input> get_consumer_input(void) const {
      if (type_ == type::consumer_input) {
        return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::consumer_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input> get_apple_vendor_top_case_input(void) const {
      if (type_ == type::apple_vendor_top_case_input) {
        return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input> get_apple_vendor_keyboard_input(void) const {
      if (type_ == type::apple_vendor_keyboard_input) {
        return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner_virtual_hid_device::hid_report::pointing_input> get_pointing_input(void) const {
      if (type_ == type::pointing_input) {
        return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::pointing_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<std::string> get_shell_command(void) const {
      if (type_ == type::shell_command) {
        return boost::get<std::string>(value_);
      }
      return std::nullopt;
    }

    std::optional<std::vector<input_source_selector>> get_input_source_selectors(void) const {
      if (type_ == type::select_input_source) {
        return boost::get<std::vector<input_source_selector>>(value_);
      }
      return std::nullopt;
    }

    absolute_time_point get_time_stamp(void) const {
      return time_stamp_;
    }

    bool operator==(const event& other) const {
      return type_ == other.type_ &&
             value_ == other.value_ &&
             time_stamp_ == other.time_stamp_;
    }

  private:
    event(void) {
    }

    static const char* to_c_string(type t) {
#define TO_C_STRING(TYPE) \
  case type::TYPE:        \
    return #TYPE;

      switch (t) {
        TO_C_STRING(keyboard_input);
        TO_C_STRING(consumer_input);
        TO_C_STRING(apple_vendor_top_case_input);
        TO_C_STRING(apple_vendor_keyboard_input);
        TO_C_STRING(pointing_input);
        TO_C_STRING(shell_command);
        TO_C_STRING(select_input_source);
      }

      return nullptr;
    }

    type type_;
    boost::variant<pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input,
                   pqrs::karabiner_virtual_hid_device::hid_report::consumer_input,
                   pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input,
                   pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input,
                   pqrs::karabiner_virtual_hid_device::hid_report::pointing_input,
                   std::string,                       // For shell_command
                   std::vector<input_source_selector> // For select_input_source
                   >
        value_;
    absolute_time_point time_stamp_;
  };

  queue(void) : dispatcher_client(),
                last_event_type_(event_type::single),
                last_event_time_stamp_(0) {
  }

  virtual ~queue(void) {
    detach_from_dispatcher();
  }

  const std::vector<event>& get_events(void) const {
    return events_;
  }

  const keyboard_repeat_detector& get_keyboard_repeat_detector(void) const {
    return keyboard_repeat_detector_;
  }

  void emplace_back_key_event(hid_usage_page hid_usage_page,
                              hid_usage hid_usage,
                              event_type event_type,
                              absolute_time_point time_stamp) {
    adjust_time_stamp(time_stamp,
                      event_type,
                      types::make_modifier_flag(hid_usage_page, hid_usage) != std::nullopt);

    switch (hid_usage_page) {
      case hid_usage_page::keyboard_or_keypad: {
        bool modify_keys = true;

        if (auto m = types::make_modifier_flag(hid_usage_page, hid_usage)) {
          if (auto modifier = types::make_hid_report_modifier(*m)) {
            switch (event_type) {
              case event_type::key_down:
                keyboard_input_.modifiers.insert(*modifier);
                modify_keys = false;
                break;

              case event_type::key_up:
                keyboard_input_.modifiers.erase(*modifier);
                modify_keys = false;
                break;

              case event_type::single:
                // Do nothing
                break;
            }
          }
        }

        if (modify_keys) {
          switch (event_type) {
            case event_type::key_down:
              keyboard_input_.keys.insert(static_cast<uint8_t>(hid_usage));
              break;

            case event_type::key_up:
              keyboard_input_.keys.erase(static_cast<uint8_t>(hid_usage));
              break;

            case event_type::single:
              // Do nothing
              break;
          }
        }

        events_.emplace_back(keyboard_input_, time_stamp);
        break;
      }

      case hid_usage_page::consumer:
        switch (event_type) {
          case event_type::key_down:
            consumer_input_.keys.insert(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::key_up:
            consumer_input_.keys.erase(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::single:
            // Do nothing
            break;
        }

        events_.emplace_back(consumer_input_, time_stamp);
        break;

      case hid_usage_page::apple_vendor_top_case:
        switch (event_type) {
          case event_type::key_down:
            apple_vendor_top_case_input_.keys.insert(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::key_up:
            apple_vendor_top_case_input_.keys.erase(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::single:
            // Do nothing
            break;
        }

        events_.emplace_back(apple_vendor_top_case_input_, time_stamp);
        break;

      case hid_usage_page::apple_vendor_keyboard:
        switch (event_type) {
          case event_type::key_down:
            apple_vendor_keyboard_input_.keys.insert(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::key_up:
            apple_vendor_keyboard_input_.keys.erase(static_cast<uint8_t>(hid_usage));
            break;

          case event_type::single:
            // Do nothing
            break;
        }

        events_.emplace_back(apple_vendor_keyboard_input_, time_stamp);
        break;

      case hid_usage_page::zero:
      case hid_usage_page::generic_desktop:
      case hid_usage_page::leds:
      case hid_usage_page::button:
        // Do nothing
        break;
    }

    keyboard_repeat_detector_.set(hid_usage_page, hid_usage, event_type);
  }

  void emplace_back_pointing_input(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
                                   event_type event_type,
                                   absolute_time_point time_stamp) {
    adjust_time_stamp(time_stamp, event_type);

    events_.emplace_back(pointing_input,
                         time_stamp);
  }

  void push_back_shell_command_event(const std::string& shell_command,
                                     absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_shell_command_event(shell_command,
                                             time_stamp);

    events_.push_back(e);
  }

  void push_back_select_input_source_event(const std::vector<input_source_selector>& input_source_selectors,
                                           absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_select_input_source_event(input_source_selectors,
                                                   time_stamp);

    events_.push_back(e);
  }

  bool empty(void) const {
    return events_.empty();
  }

  void async_post_events(std::weak_ptr<virtual_hid_device_client> weak_virtual_hid_device_client,
                         std::weak_ptr<console_user_server_client> weak_console_user_server_client) {
    enqueue_to_dispatcher(
        [this, weak_virtual_hid_device_client, weak_console_user_server_client] {
          auto now = time_utility::mach_absolute_time_point();

          while (!events_.empty()) {
            auto& e = events_.front();
            if (e.get_time_stamp() > now) {
              // If e.get_time_stamp() is too large, we reduce the delay to 3 seconds.

              absolute_time_duration duration(0);
              if (now < e.get_time_stamp()) {
                duration = std::min(e.get_time_stamp() - now,
                                    time_utility::to_absolute_time_duration(std::chrono::milliseconds(3000)));
              }

              enqueue_to_dispatcher(
                  [this, weak_virtual_hid_device_client, weak_console_user_server_client] {
                    async_post_events(weak_virtual_hid_device_client,
                                      weak_console_user_server_client);
                  },
                  when_now() + time_utility::to_milliseconds(duration));

              return;
            }

            if (auto input = e.get_keyboard_input()) {
              if (auto client = weak_virtual_hid_device_client.lock()) {
                client->async_post_keyboard_input_report(*input);
              }
            }
            if (auto input = e.get_consumer_input()) {
              if (auto client = weak_virtual_hid_device_client.lock()) {
                client->async_post_keyboard_input_report(*input);
              }
            }
            if (auto input = e.get_apple_vendor_top_case_input()) {
              if (auto client = weak_virtual_hid_device_client.lock()) {
                client->async_post_keyboard_input_report(*input);
              }
            }
            if (auto input = e.get_apple_vendor_keyboard_input()) {
              if (auto client = weak_virtual_hid_device_client.lock()) {
                client->async_post_keyboard_input_report(*input);
              }
            }
            if (auto pointing_input = e.get_pointing_input()) {
              if (auto client = weak_virtual_hid_device_client.lock()) {
                client->async_post_pointing_input_report(*pointing_input);
              }
            }
            if (auto shell_command = e.get_shell_command()) {
              try {
                if (auto client = weak_console_user_server_client.lock()) {
                  client->async_shell_command_execution(*shell_command);
                }
              } catch (std::exception& e) {
                logger::get_logger()->error("error in shell_command: {0}", e.what());
              }
            }
            if (auto input_source_selectors = e.get_input_source_selectors()) {
              if (auto client = weak_console_user_server_client.lock()) {
                auto input_source_specifiers = std::make_shared<std::vector<pqrs::osx::input_source_selector::specifier>>();
                for (const auto& s : *input_source_selectors) {
                  pqrs::osx::input_source_selector::specifier specifier;

                  if (auto& v = s.get_language_string()) {
                    specifier.set_language(*v);
                  }

                  if (auto& v = s.get_input_source_id_string()) {
                    specifier.set_input_source_id(*v);
                  }

                  if (auto& v = s.get_input_mode_id_string()) {
                    specifier.set_input_mode_id(*v);
                  }

                  input_source_specifiers->push_back(specifier);
                }
                client->async_select_input_source(input_source_specifiers);
              }
            }

            events_.erase(std::begin(events_));
          }
        });
  }

  void clear(void) {
    events_.clear();
    keyboard_repeat_detector_.clear();
  }

private:
  void adjust_time_stamp(absolute_time_point& time_stamp,
                         event_type et,
                         bool is_modifier_key_event = false) {
    // Wait is 5 milliseconds
    //
    // Note:
    // * If wait is 1 millisecond, Google Chrome issue below is sometimes happen.
    //

    auto wait = time_utility::to_absolute_time_duration(std::chrono::milliseconds(5));

    bool skip = false;
    switch (et) {
      case event_type::key_down:
        break;

      case event_type::key_up:
        if (last_event_type_ == event_type::key_up && !is_modifier_key_event) {
          skip = true;
        }
        break;

      case event_type::single:
        skip = true;
        break;
    }

    if (!skip) {
      if (time_stamp < last_event_time_stamp_ + wait) {
        time_stamp = last_event_time_stamp_ + wait;
      }
      last_event_type_ = et;
    }

    if (last_event_time_stamp_ < time_stamp) {
      last_event_time_stamp_ = time_stamp;
    }
  }

  std::vector<event> events_;

  keyboard_repeat_detector keyboard_repeat_detector_;

  // We should add a wait before `key_down` and `key_up just after key_down` in order to
  // ensure window system handles events by properly order.
  //
  // Example:
  //
  //   01. left_shift key_down
  //   02. [wait]
  //       left_control key_down
  //   03. [wait]
  //       a key_down
  //   04. [wait]
  //       a key_up
  //   05. [wait]
  //       b key_down
  //   06. [wait]
  //       b key_up
  //   07. left_shift key_up
  //   08. left_control key_up
  //   09. [wait]
  //       button1 down
  //   10. [wait]
  //       button1 up
  //
  // Without wait, window system sometimes reorder modifier flag event.
  // it causes improperly event order such as `a,shift,control,b,button1`.
  //
  // You can confirm the actual problem in Google Chrome.
  // When sending return_or_enter when right_control is pressed alone by the following manipulator,
  // Google Chrome treats return_or_enter as right_control-return_or_enter in omnibox unless we put a wait around a modifier event.
  // (open www.<entered url>.com)
  //
  //   "from": <%= from("right_control", [], ["any"]) %>,
  //   "to": <%= to([["right_control"]]) %>,
  //   "to_if_alone": <%= to([["return_or_enter"]]) %>
  //
  //
  // We also should add a wait before `key_up of modifier key`.
  // Without wait, control-space (Select the previous input source) does not work properly.

  event_type last_event_type_;
  absolute_time_point last_event_time_stamp_;

  pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input_;
  pqrs::karabiner_virtual_hid_device::hid_report::consumer_input consumer_input_;
  pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input apple_vendor_top_case_input_;
  pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input apple_vendor_keyboard_input_;
};

inline std::ostream& operator<<(std::ostream& stream, const queue::event& event) {
  stream << event.to_json();
  return stream;
}

inline void to_json(nlohmann::json& json, const queue::event& value) {
  json = value.to_json();
}
} // namespace post_event_to_virtual_devices_detail
} // namespace details
} // namespace manipulator
} // namespace krbn
