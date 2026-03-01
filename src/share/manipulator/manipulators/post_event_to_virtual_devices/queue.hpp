#pragma once

#include "event_tap_utility.hpp"
#include "keyboard_repeat_detector.hpp"
#include "types.hpp"
#include "virtual_hid_device_utility.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <array>
#include <cmath>
#include <functional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>
#include <chrono>
#include <variant>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace post_event_to_virtual_devices {
class queue final : pqrs::dispatcher::extra::dispatcher_client {
public:
  using virtual_hid_keyboard_event_posted_callback = std::function<void(const momentary_switch_event&, event_type)>;

  enum class post_backend {
    virtual_hid_device,
    cgevent,
  };

  static constexpr int64_t cgevent_source_user_data = 0x4b41524142494e45; // "KARABINE"

  struct cgevent_pointing_input final {
    pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons buttons;
    int x = 0;
    int y = 0;
    int vertical_wheel = 0;
    int horizontal_wheel = 0;

    constexpr bool operator==(const cgevent_pointing_input&) const = default;
  };

  class event final {
  public:
    enum class type {
      keyboard_input,
      consumer_input,
      apple_vendor_top_case_input,
      apple_vendor_keyboard_input,
      generic_desktop_input,
      pointing_input,
      cgevent_pointing_input,
      shell_command,
      send_user_command,
      select_input_source,
      software_function,
    };

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input& value,
          absolute_time_point time_stamp) : type_(type::keyboard_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input& value,
          absolute_time_point time_stamp) : type_(type::consumer_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input& value,
          absolute_time_point time_stamp) : type_(type::apple_vendor_top_case_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input& value,
          absolute_time_point time_stamp) : type_(type::apple_vendor_keyboard_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::generic_desktop_input& value,
          absolute_time_point time_stamp) : type_(type::generic_desktop_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    event(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input& value,
          absolute_time_point time_stamp) : type_(type::pointing_input),
                                            value_(value),
                                            time_stamp_(time_stamp) {
    }

    static event make_cgevent_pointing_input_event(const cgevent_pointing_input& value,
                                                   absolute_time_point time_stamp) {
      event e;
      e.type_ = type::cgevent_pointing_input;
      e.value_ = value;
      e.time_stamp_ = time_stamp;
      return e;
    }

    static event make_shell_command_event(const std::string& shell_command,
                                          absolute_time_point time_stamp) {
      event e;
      e.type_ = type::shell_command;
      e.value_ = shell_command;
      e.time_stamp_ = time_stamp;
      return e;
    }

    static event make_send_user_command_event(const nlohmann::json& user_command,
                                              absolute_time_point time_stamp) {
      event e;
      e.type_ = type::send_user_command;
      e.value_ = user_command;
      e.time_stamp_ = time_stamp;
      return e;
    }

    static event make_select_input_source_event(const std::vector<pqrs::osx::input_source_selector::specifier>& input_source_specifiers,
                                                absolute_time_point time_stamp) {
      event e;
      e.type_ = type::select_input_source;
      e.value_ = input_source_specifiers;
      e.time_stamp_ = time_stamp;
      return e;
    }

    static event make_software_function_event(const software_function& software_function,
                                              absolute_time_point time_stamp) {
      event e;
      e.type_ = type::software_function;
      e.value_ = software_function;
      e.time_stamp_ = time_stamp;
      return e;
    }

    nlohmann::json to_json(void) const {
      nlohmann::json json;
      json["type"] = to_c_string(type_);
      json["time_stamp"] = pqrs::osx::chrono::make_milliseconds(time_stamp_ - absolute_time_point(0)).count();

      switch (type_) {
        case type::keyboard_input:
          if (auto v = get_keyboard_input()) {
            json["keyboard_input"]["modifiers"] = v->modifiers;
            json["keyboard_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys,
                                                                                 pqrs::hid::usage_page::keyboard_or_keypad);
          }
          break;

        case type::consumer_input:
          if (auto v = get_consumer_input()) {
            json["consumer_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys,
                                                                                 pqrs::hid::usage_page::consumer);
          }
          break;

        case type::apple_vendor_top_case_input:
          if (auto v = get_apple_vendor_top_case_input()) {
            json["apple_vendor_top_case_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys,
                                                                                              pqrs::hid::usage_page::apple_vendor_top_case);
          }
          break;

        case type::apple_vendor_keyboard_input:
          if (auto v = get_apple_vendor_keyboard_input()) {
            json["apple_vendor_keyboard_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys,
                                                                                              pqrs::hid::usage_page::apple_vendor_keyboard);
          }
          break;

        case type::generic_desktop_input:
          if (auto v = get_generic_desktop_input()) {
            json["generic_desktop_input"]["keys"] = virtual_hid_device_utility::to_json(v->keys,
                                                                                        pqrs::hid::usage_page::generic_desktop);
          }
          break;

        case type::pointing_input:
        case type::cgevent_pointing_input:
          if (auto v = get_pointing_input()) {
            json["pointing_input"]["buttons"] = v->buttons;
            json["pointing_input"]["x"] = static_cast<int>(v->x);
            json["pointing_input"]["y"] = static_cast<int>(v->y);
            json["pointing_input"]["vertical_wheel"] = static_cast<int>(v->vertical_wheel);
            json["pointing_input"]["horizontal_wheel"] = static_cast<int>(v->horizontal_wheel);
          } else if (auto v = get_cgevent_pointing_input()) {
            json["pointing_input"]["buttons"] = v->buttons;
            json["pointing_input"]["x"] = v->x;
            json["pointing_input"]["y"] = v->y;
            json["pointing_input"]["vertical_wheel"] = v->vertical_wheel;
            json["pointing_input"]["horizontal_wheel"] = v->horizontal_wheel;
          }
          break;

        case type::shell_command:
          if (auto v = get_shell_command()) {
            json["shell_command"] = *v;
          }
          break;

        case type::send_user_command:
          if (auto v = get_user_command()) {
            json["user_command"] = *v;
          }
          break;

        case type::select_input_source:
          if (auto v = get_input_source_specifiers()) {
            json["input_source_specifiers"] = *v;
          }
          break;

        case type::software_function:
          if (auto v = get_software_function()) {
            json["software_function"] = *v;
          }
          break;
      }

      return json;
    }

    type get_type(void) const {
      return type_;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input> get_keyboard_input(void) const {
      if (type_ == type::keyboard_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input> get_consumer_input(void) const {
      if (type_ == type::consumer_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input> get_apple_vendor_top_case_input(void) const {
      if (type_ == type::apple_vendor_top_case_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input> get_apple_vendor_keyboard_input(void) const {
      if (type_ == type::apple_vendor_keyboard_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::generic_desktop_input> get_generic_desktop_input(void) const {
      if (type_ == type::generic_desktop_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::generic_desktop_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input> get_pointing_input(void) const {
      if (type_ == type::pointing_input) {
        return std::get<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<cgevent_pointing_input> get_cgevent_pointing_input(void) const {
      if (type_ == type::cgevent_pointing_input) {
        return std::get<cgevent_pointing_input>(value_);
      }
      return std::nullopt;
    }

    std::optional<std::string> get_shell_command(void) const {
      if (type_ == type::shell_command) {
        return std::get<std::string>(value_);
      }
      return std::nullopt;
    }

    std::optional<nlohmann::json> get_user_command(void) const {
      if (type_ == type::send_user_command) {
        return std::get<nlohmann::json>(value_);
      }
      return std::nullopt;
    }

    std::optional<std::vector<pqrs::osx::input_source_selector::specifier>> get_input_source_specifiers(void) const {
      if (type_ == type::select_input_source) {
        return std::get<std::vector<pqrs::osx::input_source_selector::specifier>>(value_);
      }
      return std::nullopt;
    }

    std::optional<software_function> get_software_function(void) const {
      if (type_ == type::software_function) {
        return std::get<software_function>(value_);
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

    static const char* _Nullable to_c_string(type t) {
#define TO_C_STRING(TYPE) \
  case type::TYPE:        \
    return #TYPE;

      switch (t) {
        TO_C_STRING(keyboard_input);
        TO_C_STRING(consumer_input);
        TO_C_STRING(apple_vendor_top_case_input);
        TO_C_STRING(apple_vendor_keyboard_input);
        TO_C_STRING(generic_desktop_input);
        TO_C_STRING(pointing_input);
        TO_C_STRING(cgevent_pointing_input);
        TO_C_STRING(shell_command);
        TO_C_STRING(send_user_command);
        TO_C_STRING(select_input_source);
        TO_C_STRING(software_function);
      }

      return nullptr;
    }

    type type_;
    std::variant<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input,
                 pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input,
                 pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input,
                 pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input,
                 pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::generic_desktop_input,
                 pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input,
                 cgevent_pointing_input,
                 std::string,                                              // For shell_command
                 nlohmann::json,                                           // For send_user_command
                 std::vector<pqrs::osx::input_source_selector::specifier>, // For select_input_source
                 software_function                                         // For software_function
                 >
        value_;
    absolute_time_point time_stamp_;
  };

  queue(void) : dispatcher_client(),
                last_event_type_(event_type::single),
                last_event_time_stamp_(0) {
    cgevent_source_ = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
  }

  virtual ~queue(void) {
    detach_from_dispatcher([this] {
      if (cgevent_source_) {
        CFRelease(cgevent_source_);
        cgevent_source_ = nullptr;
      }
    });
  }

  const std::vector<event>& get_events(void) const {
    return events_;
  }

  const keyboard_repeat_detector& get_keyboard_repeat_detector(void) const {
    return keyboard_repeat_detector_;
  }

  void set_post_backend(post_backend backend) {
    post_backend_ = backend;
  }

  post_backend get_post_backend(void) const {
    return post_backend_;
  }

  void set_virtual_hid_keyboard_event_posted_callback(virtual_hid_keyboard_event_posted_callback value) {
    virtual_hid_keyboard_event_posted_callback_ = value;
  }

  void set_cgevent_double_click_settings(int interval_milliseconds,
                                         int distance) {
    cgevent_double_click_interval_ = std::chrono::milliseconds(std::max(1, interval_milliseconds));
    cgevent_double_click_distance_ = static_cast<CGFloat>(std::max(0, distance));
  }

  void emplace_back_key_event(const pqrs::hid::usage_pair& usage_pair,
                              event_type event_type,
                              absolute_time_point time_stamp) {
    momentary_switch_event mse(usage_pair);

    adjust_time_stamp(time_stamp,
                      event_type,
                      mse.modifier_flag());

    auto usage_page = usage_pair.get_usage_page();
    auto usage = usage_pair.get_usage();

    if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
      bool modify_keys = true;

      if (auto m = mse.make_modifier_flag()) {
        if (auto modifier = make_hid_report_modifier(*m)) {
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
            keyboard_input_.keys.insert(type_safe::get(usage));
            break;

          case event_type::key_up:
            keyboard_input_.keys.erase(type_safe::get(usage));
            break;

          case event_type::single:
            // Do nothing
            break;
        }
      }

      events_.push_back(event(keyboard_input_, time_stamp));

    } else if (usage_page == pqrs::hid::usage_page::consumer) {
      switch (event_type) {
        case event_type::key_down:
          consumer_input_.keys.insert(type_safe::get(usage));
          break;

        case event_type::key_up:
          consumer_input_.keys.erase(type_safe::get(usage));
          break;

        case event_type::single:
          // Do nothing
          break;
      }

      events_.push_back(event(consumer_input_, time_stamp));

    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
      switch (event_type) {
        case event_type::key_down:
          apple_vendor_top_case_input_.keys.insert(type_safe::get(usage));
          break;

        case event_type::key_up:
          apple_vendor_top_case_input_.keys.erase(type_safe::get(usage));
          break;

        case event_type::single:
          // Do nothing
          break;
      }

      events_.push_back(event(apple_vendor_top_case_input_, time_stamp));

    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
      switch (event_type) {
        case event_type::key_down:
          apple_vendor_keyboard_input_.keys.insert(type_safe::get(usage));
          break;

        case event_type::key_up:
          apple_vendor_keyboard_input_.keys.erase(type_safe::get(usage));
          break;

        case event_type::single:
          // Do nothing
          break;
      }

      events_.push_back(event(apple_vendor_keyboard_input_, time_stamp));

    } else if (usage_page == pqrs::hid::usage_page::generic_desktop) {
      switch (event_type) {
        case event_type::key_down:
          generic_desktop_input_.keys.insert(type_safe::get(usage));
          break;

        case event_type::key_up:
          generic_desktop_input_.keys.erase(type_safe::get(usage));
          break;

        case event_type::single:
          // Do nothing
          break;
      }

      events_.push_back(event(generic_desktop_input_, time_stamp));
    }

    keyboard_repeat_detector_.set(mse, event_type);
  }

  void emplace_back_pointing_input(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input& pointing_input,
                                   event_type event_type,
                                   absolute_time_point time_stamp) {
    adjust_time_stamp(time_stamp, event_type);

    if (post_backend_ == post_backend::cgevent) {
      cgevent_pointing_input cgevent_input;
      cgevent_input.buttons = pointing_input.buttons;
      cgevent_input.x = static_cast<int>(static_cast<int8_t>(pointing_input.x));
      cgevent_input.y = static_cast<int>(static_cast<int8_t>(pointing_input.y));
      cgevent_input.vertical_wheel = static_cast<int>(static_cast<int8_t>(pointing_input.vertical_wheel));
      cgevent_input.horizontal_wheel = static_cast<int>(static_cast<int8_t>(pointing_input.horizontal_wheel));
      events_.push_back(event::make_cgevent_pointing_input_event(cgevent_input,
                                                                 time_stamp));
    } else {
      events_.push_back(event(pointing_input,
                              time_stamp));
    }
  }

  void emplace_back_virtual_hid_pointing_input(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::pointing_input& pointing_input,
                                               event_type event_type,
                                               absolute_time_point time_stamp) {
    adjust_time_stamp(time_stamp, event_type);

    events_.push_back(event(pointing_input,
                            time_stamp));
  }

  void emplace_back_cgevent_pointing_input(const cgevent_pointing_input& pointing_input,
                                           event_type event_type,
                                           absolute_time_point time_stamp) {
    adjust_time_stamp(time_stamp, event_type);

    events_.push_back(event::make_cgevent_pointing_input_event(pointing_input,
                                                               time_stamp));
  }

  void push_back_shell_command_event(const std::string& shell_command,
                                     absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_shell_command_event(shell_command,
                                             time_stamp);

    events_.push_back(e);
  }

  void push_back_send_user_command_event(const nlohmann::json& user_command,
                                         absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_send_user_command_event(user_command,
                                                 time_stamp);

    events_.push_back(e);
  }

  void push_back_select_input_source_event(const std::vector<pqrs::osx::input_source_selector::specifier>& input_source_specifiers,
                                           absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_select_input_source_event(input_source_specifiers,
                                                   time_stamp);

    events_.push_back(e);
  }

  void push_back_software_function_event(const software_function& software_function,
                                         absolute_time_point time_stamp) {
    // Do not call adjust_time_stamp.
    auto e = event::make_software_function_event(software_function,
                                                 time_stamp);

    events_.push_back(e);
  }

  bool empty(void) const {
    return events_.empty();
  }

  void async_post_events(std::weak_ptr<pqrs::karabiner::driverkit::virtual_hid_device_service::client> weak_virtual_hid_device_service_client,
                         std::weak_ptr<console_user_server_client> weak_console_user_server_client) {
    enqueue_to_dispatcher(
        [this, weak_virtual_hid_device_service_client, weak_console_user_server_client] {
          auto now = pqrs::osx::chrono::mach_absolute_time_point();

          while (!events_.empty()) {
            auto& e = events_.front();
            if (e.get_time_stamp() > now) {
              // If e.get_time_stamp() is too large, we reduce the delay to 3 seconds.

              absolute_time_duration duration(0);
              if (now < e.get_time_stamp()) {
                duration = std::min(e.get_time_stamp() - now,
                                    pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(3000)));
              }

              enqueue_to_dispatcher(
                  [this, weak_virtual_hid_device_service_client, weak_console_user_server_client] {
                    async_post_events(weak_virtual_hid_device_service_client,
                                      weak_console_user_server_client);
                  },
                  when_now() + pqrs::osx::chrono::make_milliseconds(duration));

              return;
            }

            if (auto input = e.get_keyboard_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                emit_virtual_hid_keyboard_events(previous_posted_keyboard_input_, *input);
                previous_posted_keyboard_input_ = *input;
                client->async_post_report(*input);
              }
            }
            if (auto input = e.get_consumer_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                emit_virtual_hid_consumer_events(previous_posted_consumer_input_, *input);
                previous_posted_consumer_input_ = *input;
                client->async_post_report(*input);
              }
            }
            if (auto input = e.get_apple_vendor_top_case_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                emit_virtual_hid_apple_vendor_top_case_events(previous_posted_apple_vendor_top_case_input_, *input);
                previous_posted_apple_vendor_top_case_input_ = *input;
                client->async_post_report(*input);
              }
            }
            if (auto input = e.get_apple_vendor_keyboard_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                emit_virtual_hid_apple_vendor_keyboard_events(previous_posted_apple_vendor_keyboard_input_, *input);
                previous_posted_apple_vendor_keyboard_input_ = *input;
                client->async_post_report(*input);
              }
            }
            if (auto input = e.get_generic_desktop_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                client->async_post_report(*input);
              }
            }
            if (auto pointing_input = e.get_pointing_input()) {
              if (auto client = weak_virtual_hid_device_service_client.lock()) {
                client->async_post_report(*pointing_input);
              }
            }
            if (auto pointing_input = e.get_cgevent_pointing_input()) {
              post_pointing_input_to_cgevent(*pointing_input);
            }
            if (auto shell_command = e.get_shell_command()) {
              if (auto client = weak_console_user_server_client.lock()) {
                client->async_shell_command_execution(*shell_command);
              }
            }
            if (auto user_command = e.get_user_command()) {
              if (auto client = weak_console_user_server_client.lock()) {
                client->async_send_user_command(*user_command);
              }
            }
            if (auto input_source_specifiers = e.get_input_source_specifiers()) {
              if (auto client = weak_console_user_server_client.lock()) {
                auto specifiers = std::make_shared<std::vector<pqrs::osx::input_source_selector::specifier>>();
                for (const auto& s : *input_source_specifiers) {
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

                  specifiers->push_back(specifier);
                }
                client->async_select_input_source(specifiers);
              }
            }
            if (auto software_function = e.get_software_function()) {
              if (auto client = weak_console_user_server_client.lock()) {
                client->async_software_function(*software_function);
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

  static CGMouseButton make_mouse_button(uint8_t button) {
    switch (button) {
      case 1:
        return kCGMouseButtonLeft;
      case 2:
        return kCGMouseButtonRight;
      default:
        return static_cast<CGMouseButton>(button - 1);
    }
  }

  static CGEventType make_button_event_type(bool down,
                                            uint8_t button) {
    if (button == 1) {
      return down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
    }
    if (button == 2) {
      return down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
    }
    return down ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
  }

  static CGEventType make_move_event_type(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons& buttons) {
    if (buttons.exists(1)) {
      return kCGEventLeftMouseDragged;
    }
    if (buttons.exists(2)) {
      return kCGEventRightMouseDragged;
    }
    for (uint8_t i = 3; i <= 32; ++i) {
      if (buttons.exists(i)) {
        return kCGEventOtherMouseDragged;
      }
    }
    return kCGEventMouseMoved;
  }

  int64_t make_click_state(uint8_t button,
                           CGPoint current_position,
                           bool down) {
    if (button == 0 || button >= button_event_numbers_.size()) {
      return 1;
    }

    if (down) {
      auto now = std::chrono::steady_clock::now();

      bool same_series = false;
      if (has_last_click_) {
        auto interval = now - last_click_time_;
        auto dx = std::fabs(current_position.x - last_click_position_.x);
        auto dy = std::fabs(current_position.y - last_click_position_.y);
        same_series = (last_click_button_ == button &&
                       interval <= cgevent_double_click_interval_ &&
                       dx <= cgevent_double_click_distance_ &&
                       dy <= cgevent_double_click_distance_);
      }

      if (same_series) {
        last_click_state_ = std::min<int64_t>(last_click_state_ + 1, 3);
      } else {
        last_click_state_ = 1;
      }

      has_last_click_ = true;
      last_click_button_ = button;
      last_click_time_ = now;
      last_click_position_ = current_position;

      ++next_mouse_event_number_;
      button_event_numbers_[button] = next_mouse_event_number_;
      button_click_states_[button] = last_click_state_;
      return last_click_state_;
    }

    if (button_click_states_[button] == 0) {
      button_click_states_[button] = 1;
    }
    if (button_event_numbers_[button] == 0) {
      ++next_mouse_event_number_;
      button_event_numbers_[button] = next_mouse_event_number_;
    }
    return button_click_states_[button];
  }

  void post_pointing_input_to_cgevent(const cgevent_pointing_input& input) {
    auto x = input.x;
    auto y = input.y;
    auto vertical_wheel = input.vertical_wheel;
    auto horizontal_wheel = input.horizontal_wheel;

    auto current_position = CGPointZero;
    if (auto p = event_tap_utility::get_latest_pointing_location()) {
      current_position = *p;
    } else if (auto event = CGEventCreate(nullptr)) {
      current_position = CGEventGetLocation(event);
      CFRelease(event);
    }

    for (uint8_t button = 1; button <= 32; ++button) {
      auto current = input.buttons.exists(button);
      auto previous = cgevent_pointing_input_.buttons.exists(button);
      if (current != previous) {
        if (auto event = CGEventCreateMouseEvent(nullptr,
                                                 make_button_event_type(current, button),
                                                 current_position,
                                                 make_mouse_button(button))) {
          auto click_state = make_click_state(button,
                                              current_position,
                                              current);
          CGEventSetIntegerValueField(event,
                                      kCGMouseEventClickState,
                                      click_state);
          CGEventSetIntegerValueField(event,
                                      kCGMouseEventNumber,
                                      button_event_numbers_[button]);
          CGEventSetIntegerValueField(event,
                                      kCGEventSourceUserData,
                                      cgevent_source_user_data);
          CGEventPost(kCGHIDEventTap, event);
          CFRelease(event);
        }
      }
    }

    if (x != 0 || y != 0) {
      if (auto event = CGEventCreateMouseEvent(nullptr,
                                               make_move_event_type(input.buttons),
                                               current_position,
                                               kCGMouseButtonLeft)) {
        CGEventSetIntegerValueField(event, kCGMouseEventDeltaX, x);
        CGEventSetIntegerValueField(event, kCGMouseEventDeltaY, y);
        CGEventSetIntegerValueField(event,
                                    kCGEventSourceUserData,
                                    cgevent_source_user_data);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
      }
    }

    if (vertical_wheel != 0 || horizontal_wheel != 0) {
      if (auto event = CGEventCreateScrollWheelEvent(nullptr,
                                                     kCGScrollEventUnitLine,
                                                     2,
                                                     vertical_wheel,
                                                     horizontal_wheel)) {
        CGEventSetIntegerValueField(event,
                                    kCGEventSourceUserData,
                                    cgevent_source_user_data);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
      }
    }

    cgevent_pointing_input_ = input;
  }

  void adjust_time_stamp(absolute_time_point& time_stamp,
                         event_type et,
                         bool is_modifier_key_event = false) {
    // Wait is 5 milliseconds
    //
    // Note:
    // * If wait is 1 millisecond, Google Chrome issue below is sometimes happen.
    //

    auto wait = pqrs::osx::chrono::make_absolute_time_duration(std::chrono::milliseconds(5));

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
  virtual_hid_keyboard_event_posted_callback virtual_hid_keyboard_event_posted_callback_;

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

  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input keyboard_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input previous_posted_keyboard_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input consumer_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input previous_posted_consumer_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input apple_vendor_top_case_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input previous_posted_apple_vendor_top_case_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input apple_vendor_keyboard_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input previous_posted_apple_vendor_keyboard_input_;
  pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::generic_desktop_input generic_desktop_input_;
  post_backend post_backend_ = post_backend::virtual_hid_device;

  cgevent_pointing_input cgevent_pointing_input_;
  CGEventSourceRef _Nullable cgevent_source_;
  std::chrono::milliseconds cgevent_double_click_interval_{500};
  CGFloat cgevent_double_click_distance_ = 4.0;
  bool has_last_click_ = false;
  uint8_t last_click_button_ = 0;
  int64_t last_click_state_ = 0;
  CGPoint last_click_position_ = CGPointZero;
  std::chrono::steady_clock::time_point last_click_time_;
  int64_t next_mouse_event_number_ = 0;
  std::array<int64_t, 33> button_event_numbers_{};
  std::array<int64_t, 33> button_click_states_{};

  std::optional<momentary_switch_event> make_momentary_switch_event_from_modifier(
      pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifier modifier) const {
    using hid_report_modifier = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifier;

    switch (modifier) {
      case hid_report_modifier::left_control:
        return momentary_switch_event(modifier_flag::left_control);
      case hid_report_modifier::left_shift:
        return momentary_switch_event(modifier_flag::left_shift);
      case hid_report_modifier::left_option:
        return momentary_switch_event(modifier_flag::left_option);
      case hid_report_modifier::left_command:
        return momentary_switch_event(modifier_flag::left_command);
      case hid_report_modifier::right_control:
        return momentary_switch_event(modifier_flag::right_control);
      case hid_report_modifier::right_shift:
        return momentary_switch_event(modifier_flag::right_shift);
      case hid_report_modifier::right_option:
        return momentary_switch_event(modifier_flag::right_option);
      case hid_report_modifier::right_command:
        return momentary_switch_event(modifier_flag::right_command);
    }
  }

  void emit_virtual_hid_keyboard_events(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input& previous,
                                        const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::keyboard_input& current) {
    if (!virtual_hid_keyboard_event_posted_callback_) {
      return;
    }

    using hid_report_modifier = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifier;

    static constexpr std::array<hid_report_modifier, 8> modifiers = {
        hid_report_modifier::left_control,
        hid_report_modifier::left_shift,
        hid_report_modifier::left_option,
        hid_report_modifier::left_command,
        hid_report_modifier::right_control,
        hid_report_modifier::right_shift,
        hid_report_modifier::right_option,
        hid_report_modifier::right_command,
    };

    for (const auto& modifier : modifiers) {
      auto previous_exists = previous.modifiers.exists(modifier);
      auto current_exists = current.modifiers.exists(modifier);

      if (previous_exists == current_exists) {
        continue;
      }

      if (auto m = make_momentary_switch_event_from_modifier(modifier)) {
        virtual_hid_keyboard_event_posted_callback_(*m,
                                                    current_exists ? event_type::key_down : event_type::key_up);
      }
    }

    for (const auto& key : previous.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!current.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_up);
      }
    }

    for (const auto& key : current.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!previous.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_down);
      }
    }
  }

  void emit_virtual_hid_apple_vendor_keyboard_events(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input& previous,
                                                     const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_keyboard_input& current) {
    if (!virtual_hid_keyboard_event_posted_callback_) {
      return;
    }

    for (const auto& key : previous.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!current.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_up);
      }
    }

    for (const auto& key : current.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!previous.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_down);
      }
    }
  }

  void emit_virtual_hid_consumer_events(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input& previous,
                                        const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::consumer_input& current) {
    if (!virtual_hid_keyboard_event_posted_callback_) {
      return;
    }

    for (const auto& key : previous.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!current.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_up);
      }
    }

    for (const auto& key : current.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!previous.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::consumer,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_down);
      }
    }
  }

  void emit_virtual_hid_apple_vendor_top_case_events(const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input& previous,
                                                      const pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::apple_vendor_top_case_input& current) {
    if (!virtual_hid_keyboard_event_posted_callback_) {
      return;
    }

    for (const auto& key : previous.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!current.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_up);
      }
    }

    for (const auto& key : current.keys.get_raw_value()) {
      if (key == 0) {
        continue;
      }

      if (!previous.keys.exists(key)) {
        virtual_hid_keyboard_event_posted_callback_(
            momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                   pqrs::hid::usage::value_t(key)),
            event_type::key_down);
      }
    }
  }
};

inline std::ostream& operator<<(std::ostream& stream, const queue::event& event) {
  stream << event.to_json();
  return stream;
}

inline void to_json(nlohmann::json& json, const queue::event& value) {
  json = value.to_json();
}
} // namespace post_event_to_virtual_devices
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
