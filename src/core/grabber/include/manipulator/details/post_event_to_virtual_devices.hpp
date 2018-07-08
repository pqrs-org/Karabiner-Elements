#pragma once

#include "boost_defs.hpp"

#include "console_user_server_client.hpp"
#include "keyboard_repeat_detector.hpp"
#include "krbn_notification_center.hpp"
#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "stream_utility.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include "virtual_hid_device_utility.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <mach/mach_time.h>

namespace krbn {
namespace manipulator {
namespace details {
class post_event_to_virtual_devices final : public base {
public:
  class queue final {
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
            uint64_t time_stamp) : type_(type::keyboard_input),
                                   value_(value),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::consumer_input& value,
            uint64_t time_stamp) : type_(type::consumer_input),
                                   value_(value),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input& value,
            uint64_t time_stamp) : type_(type::apple_vendor_top_case_input),
                                   value_(value),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input& value,
            uint64_t time_stamp) : type_(type::apple_vendor_keyboard_input),
                                   value_(value),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& value,
            uint64_t time_stamp) : type_(type::pointing_input),
                                   value_(value),
                                   time_stamp_(time_stamp) {
      }

      static event make_shell_command_event(const std::string& shell_command,
                                            uint64_t time_stamp) {
        event e;
        e.type_ = type::shell_command;
        e.value_ = shell_command;
        e.time_stamp_ = time_stamp;
        return e;
      }

      static event make_select_input_source_event(const std::vector<input_source_selector>& input_source_selector,
                                                  uint64_t time_stamp) {
        event e;
        e.type_ = type::select_input_source;
        e.value_ = input_source_selector;
        e.time_stamp_ = time_stamp;
        return e;
      }

      nlohmann::json to_json(void) const {
        nlohmann::json json;
        json["type"] = to_c_string(type_);
        json["time_stamp"] = time_stamp_;

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

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input> get_keyboard_input(void) const {
        if (type_ == type::keyboard_input) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input>(value_);
        }
        return boost::none;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::consumer_input> get_consumer_input(void) const {
        if (type_ == type::consumer_input) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::consumer_input>(value_);
        }
        return boost::none;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input> get_apple_vendor_top_case_input(void) const {
        if (type_ == type::apple_vendor_top_case_input) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input>(value_);
        }
        return boost::none;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input> get_apple_vendor_keyboard_input(void) const {
        if (type_ == type::apple_vendor_keyboard_input) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input>(value_);
        }
        return boost::none;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::pointing_input> get_pointing_input(void) const {
        if (type_ == type::pointing_input) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_report::pointing_input>(value_);
        }
        return boost::none;
      }

      boost::optional<std::string> get_shell_command(void) const {
        if (type_ == type::shell_command) {
          return boost::get<std::string>(value_);
        }
        return boost::none;
      }

      boost::optional<std::vector<input_source_selector>> get_input_source_selectors(void) const {
        if (type_ == type::select_input_source) {
          return boost::get<std::vector<input_source_selector>>(value_);
        }
        return boost::none;
      }

      uint64_t get_time_stamp(void) const {
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
      uint64_t time_stamp_;
    };

    queue(void) : last_event_type_(event_type::single),
                  last_event_time_stamp_(0) {
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
                                uint64_t time_stamp) {
      adjust_time_stamp(time_stamp,
                        event_type,
                        types::make_modifier_flag(hid_usage_page, hid_usage) != boost::none);

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
                                     uint64_t time_stamp) {
      adjust_time_stamp(time_stamp, event_type);

      events_.emplace_back(pointing_input,
                           time_stamp);
    }

    void push_back_shell_command_event(const std::string& shell_command,
                                       uint64_t time_stamp) {
      // Do not call adjust_time_stamp.
      auto e = event::make_shell_command_event(shell_command,
                                               time_stamp);

      events_.push_back(e);
    }

    void push_back_select_input_source_event(const std::vector<input_source_selector>& input_source_selectors,
                                             uint64_t time_stamp) {
      // Do not call adjust_time_stamp.
      auto e = event::make_select_input_source_event(input_source_selectors,
                                                     time_stamp);

      events_.push_back(e);
    }

    bool empty(void) const {
      return events_.empty();
    }

    void post_events(virtual_hid_device_client& virtual_hid_device_client) {
      if (timer_ && timer_->fired()) {
        timer_ = nullptr;
      }
      if (timer_) {
        return;
      }

      uint64_t now = mach_absolute_time();

      while (!events_.empty()) {
        auto& e = events_.front();
        if (e.get_time_stamp() > now) {
          // If e.get_time_stamp() is too large, we reduce the delay to 3 seconds.
          auto when = std::min(e.get_time_stamp(), now + time_utility::nano_to_absolute(3 * NSEC_PER_SEC));

          timer_ = std::make_unique<gcd_utility::main_queue_after_timer>(when,
                                                                         true,
                                                                         ^{
                                                                           post_events(virtual_hid_device_client);
                                                                         });
          return;
        }

        if (auto input = e.get_keyboard_input()) {
          virtual_hid_device_client.post_keyboard_input_report(*input);
        }
        if (auto input = e.get_consumer_input()) {
          virtual_hid_device_client.post_keyboard_input_report(*input);
        }
        if (auto input = e.get_apple_vendor_top_case_input()) {
          virtual_hid_device_client.post_keyboard_input_report(*input);
        }
        if (auto input = e.get_apple_vendor_keyboard_input()) {
          virtual_hid_device_client.post_keyboard_input_report(*input);
        }
        if (auto pointing_input = e.get_pointing_input()) {
          virtual_hid_device_client.post_pointing_input_report(*pointing_input);
        }
        if (auto shell_command = e.get_shell_command()) {
          try {
            if (auto current_console_user_id = session::get_current_console_user_id()) {
              console_user_server_client client(*current_console_user_id);
              client.shell_command_execution(*shell_command);
            }
          } catch (std::exception& e) {
            logger::get_logger().error("error in shell_command: {0}", e.what());
          }
        }
        if (auto input_source_selectors = e.get_input_source_selectors()) {
          try {
            if (auto current_console_user_id = session::get_current_console_user_id()) {
              console_user_server_client client(*current_console_user_id);
              for (const auto& s : *input_source_selectors) {
                client.select_input_source(s, now);
              }
            }
          } catch (std::exception& e) {
            logger::get_logger().error("error in select_input_source: {0}", e.what());
          }
        }

        events_.erase(std::begin(events_));
      }
    }

    void clear(void) {
      events_.clear();
      keyboard_repeat_detector_.clear();
    }

  private:
    void adjust_time_stamp(uint64_t& time_stamp,
                           event_type et,
                           bool is_modifier_key_event = false) {
      // Wait is 5 milliseconds
      //
      // Note:
      // * If wait is 1 millisecond, Google Chrome issue below is sometimes happen.
      //

      auto wait = time_utility::nano_to_absolute(5 * NSEC_PER_MSEC);

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
    std::unique_ptr<gcd_utility::main_queue_after_timer> timer_;

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
    uint64_t last_event_time_stamp_;

    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input_;
    pqrs::karabiner_virtual_hid_device::hid_report::consumer_input consumer_input_;
    pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input apple_vendor_top_case_input_;
    pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input apple_vendor_keyboard_input_;
  };

  class key_event_dispatcher final {
  public:
    void dispatch_key_down_event(device_id device_id,
                                 hid_usage_page hid_usage_page,
                                 hid_usage hid_usage,
                                 queue& queue,
                                 uint64_t time_stamp) {
      // Enqueue key_down event if it is not sent yet.

      if (!key_event_exists(hid_usage_page, hid_usage)) {
        pressed_keys_.emplace_back(device_id, std::make_pair(hid_usage_page, hid_usage));
        enqueue_key_event(hid_usage_page, hid_usage, event_type::key_down, queue, time_stamp);
      }
    }

    void dispatch_key_up_event(hid_usage_page hid_usage_page,
                               hid_usage hid_usage,
                               queue& queue,
                               uint64_t time_stamp) {
      // Enqueue key_up event if it is already sent.

      if (key_event_exists(hid_usage_page, hid_usage)) {
        pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                           std::end(pressed_keys_),
                                           [&](auto& k) {
                                             return k.second.first == hid_usage_page &&
                                                    k.second.second == hid_usage;
                                           }),
                            std::end(pressed_keys_));
        enqueue_key_event(hid_usage_page, hid_usage, event_type::key_up, queue, time_stamp);
      }
    }

    void dispatch_modifier_key_event(const modifier_flag_manager& modifier_flag_manager,
                                     queue& queue,
                                     uint64_t time_stamp) {
      auto modifier_flags = {
          modifier_flag::left_control,
          modifier_flag::left_shift,
          modifier_flag::left_option,
          modifier_flag::left_command,
          modifier_flag::right_control,
          modifier_flag::right_shift,
          modifier_flag::right_option,
          modifier_flag::right_command,
          modifier_flag::fn,
      };
      for (const auto& m : modifier_flags) {
        bool pressed = pressed_modifier_flags_.find(m) != std::end(pressed_modifier_flags_);

        if (modifier_flag_manager.is_pressed(m)) {
          if (!pressed) {
            if (auto key_code = types::make_key_code(m)) {
              if (auto hid_usage_page = types::make_hid_usage_page(*key_code)) {
                if (auto hid_usage = types::make_hid_usage(*key_code)) {
                  enqueue_key_event(*hid_usage_page, *hid_usage, event_type::key_down, queue, time_stamp);
                }
              }
            }
            pressed_modifier_flags_.insert(m);
          }

        } else {
          if (pressed) {
            if (auto key_code = types::make_key_code(m)) {
              if (auto hid_usage_page = types::make_hid_usage_page(*key_code)) {
                if (auto hid_usage = types::make_hid_usage(*key_code)) {
                  enqueue_key_event(*hid_usage_page, *hid_usage, event_type::key_up, queue, time_stamp);
                }
              }
            }
            pressed_modifier_flags_.erase(m);
          }
        }
      }
    }

    void dispatch_key_up_events_by_device_id(device_id device_id,
                                             queue& queue,
                                             uint64_t time_stamp) {
      while (true) {
        bool found = false;
        for (const auto& k : pressed_keys_) {
          if (k.first == device_id) {
            found = true;
            dispatch_key_up_event(k.second.first,
                                  k.second.second,
                                  queue,
                                  time_stamp);
            break;
          }
        }
        if (!found) {
          break;
        }
      }
    }

    const std::vector<std::pair<device_id, std::pair<hid_usage_page, hid_usage>>>& get_pressed_keys(void) const {
      return pressed_keys_;
    }

  private:
    bool key_event_exists(hid_usage_page usage_page,
                          hid_usage usage) {
      auto it = std::find_if(std::begin(pressed_keys_),
                             std::end(pressed_keys_),
                             [&](auto& k) {
                               return k.second.first == usage_page &&
                                      k.second.second == usage;
                             });
      return (it != std::end(pressed_keys_));
    }

    void enqueue_key_event(hid_usage_page usage_page,
                           hid_usage usage,
                           event_type event_type,
                           queue& queue,
                           uint64_t time_stamp) {
      queue.emplace_back_key_event(usage_page, usage, event_type, time_stamp);
    }

    std::vector<std::pair<device_id, std::pair<hid_usage_page, hid_usage>>> pressed_keys_;
    std::unordered_set<modifier_flag> pressed_modifier_flags_;
  };

  class mouse_key_handler final {
  public:
    class count_converter final {
    public:
      count_converter(int threshold) : threshold_(threshold),
                                       count_(0) {
      }

      uint8_t update(int value) {
        int result = 0;

        count_ += value;

        while (count_ <= -threshold_) {
          --result;
          count_ += threshold_;
        }
        while (count_ >= threshold_) {
          ++result;
          count_ -= threshold_;
        }

        return static_cast<uint8_t>(result);
      }

      void reset(void) {
        count_ = 0;
      }

    private:
      int threshold_;
      int count_;
    };

    mouse_key_handler(queue& queue,
                      const system_preferences& system_preferences) : queue_(queue),
                                                                      system_preferences_(system_preferences),
                                                                      x_count_converter_(128),
                                                                      y_count_converter_(128),
                                                                      vertical_wheel_count_converter_(128),
                                                                      horizontal_wheel_count_converter_(128) {
    }

    void manipulator_timer_invoked(manipulator_timer::timer_id timer_id, uint64_t now) {
      if (timer_id == manipulator_timer_id_) {
        manipulator_timer_id_ = boost::none;
        post_event(now);
        krbn_notification_center::get_instance().input_event_arrived();
      }
    }

    void push_back_mouse_key(device_id device_id,
                             const mouse_key& mouse_key,
                             const std::shared_ptr<event_queue>& output_event_queue,
                             uint64_t time_stamp) {
      erase_entry(device_id, mouse_key);
      entries_.emplace_back(device_id, mouse_key);

      output_event_queue_ = output_event_queue;

      post_event(time_stamp);
    }

    void erase_mouse_key(device_id device_id,
                         const mouse_key& mouse_key,
                         const std::shared_ptr<event_queue>& output_event_queue,
                         uint64_t time_stamp) {
      erase_entry(device_id, mouse_key);

      output_event_queue_ = output_event_queue;

      post_event(time_stamp);
    }

    void erase_mouse_keys_by_device_id(device_id device_id,
                                       uint64_t time_stamp) {
      entries_.erase(std::remove_if(std::begin(entries_),
                                    std::end(entries_),
                                    [&](const auto& pair) {
                                      return pair.first == device_id;
                                    }),
                     std::end(entries_));

      post_event(time_stamp);
    }

    bool active(void) const {
      return !entries_.empty();
    }

  private:
    void erase_entry(device_id device_id,
                     const mouse_key& mouse_key) {
      entries_.erase(std::remove_if(std::begin(entries_),
                                    std::end(entries_),
                                    [&](const auto& pair) {
                                      return pair.first == device_id &&
                                             pair.second == mouse_key;
                                    }),
                     std::end(entries_));
    }

    void post_event(uint64_t time_stamp) {
      if (auto oeq = output_event_queue_.lock()) {
        mouse_key total;
        for (const auto& pair : entries_) {
          total += pair.second;
        }

        if (!system_preferences_.get_swipe_scroll_direction()) {
          total.invert_wheel();
        }

        if (total.is_zero()) {
          manipulator_timer_id_ = boost::none;
          last_mouse_key_total_ = boost::none;

        } else {
          if (last_mouse_key_total_ != total) {
            last_mouse_key_total_ = total;

            x_count_converter_.reset();
            y_count_converter_.reset();
            vertical_wheel_count_converter_.reset();
            horizontal_wheel_count_converter_.reset();
          }

          pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
          report.buttons = oeq->get_pointing_button_manager().make_hid_report_buttons();
          report.x = x_count_converter_.update(static_cast<int>(total.get_x() * total.get_speed_multiplier()));
          report.y = y_count_converter_.update(static_cast<int>(total.get_y() * total.get_speed_multiplier()));
          report.vertical_wheel = vertical_wheel_count_converter_.update(static_cast<int>(total.get_vertical_wheel() * total.get_speed_multiplier()));
          report.horizontal_wheel = horizontal_wheel_count_converter_.update(static_cast<int>(total.get_horizontal_wheel() * total.get_speed_multiplier()));

          queue_.emplace_back_pointing_input(report,
                                             event_type::single,
                                             time_stamp);

          uint64_t delay_milliseconds = 20;
          auto when = time_stamp + time_utility::nano_to_absolute(delay_milliseconds * NSEC_PER_MSEC);
          manipulator_timer_id_ = manipulator_timer::get_instance().add_entry(when);
        }
      }
    }

    queue& queue_;
    const system_preferences& system_preferences_;
    std::vector<std::pair<device_id, mouse_key>> entries_;
    std::weak_ptr<event_queue> output_event_queue_;
    boost::optional<manipulator_timer::timer_id> manipulator_timer_id_;
    boost::optional<mouse_key> last_mouse_key_total_;
    count_converter x_count_converter_;
    count_converter y_count_converter_;
    count_converter vertical_wheel_count_converter_;
    count_converter horizontal_wheel_count_converter_;
  };

  post_event_to_virtual_devices(const system_preferences& system_preferences) : base(),
                                                                                queue_(),
                                                                                mouse_key_handler_(queue_,
                                                                                                   system_preferences) {
  }

  virtual ~post_event_to_virtual_devices(void) {
  }

  virtual bool already_manipulated(const event_queue::queued_event& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::queued_event& front_input_event,
                                       const event_queue& input_event_queue,
                                       const std::shared_ptr<event_queue>& output_event_queue,
                                       uint64_t now) {
    if (output_event_queue) {
      if (!front_input_event.get_valid()) {
        return manipulate_result::passed;
      }

      output_event_queue->push_back_event(front_input_event);
      front_input_event.set_valid(false);

      // Dispatch modifier key event only when front_input_event is key_down or modifier key.

      bool dispatch_modifier_key_event = false;
      bool dispatch_modifier_key_event_before = false;
      {
        boost::optional<modifier_flag> m;
        if (auto key_code = front_input_event.get_event().get_key_code()) {
          m = types::make_modifier_flag(*key_code);
        }

        if (m) {
          // front_input_event is modifier key event.
          if (!front_input_event.get_lazy()) {
            dispatch_modifier_key_event = true;
            dispatch_modifier_key_event_before = false;
          }

        } else {
          if (front_input_event.get_event_type() == event_type::key_down) {
            if (front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_button) {
              if (!queue_.get_keyboard_repeat_detector().is_repeating() &&
                  pressed_buttons_.empty() &&
                  !mouse_key_handler_.active()) {
                dispatch_modifier_key_event = true;
                dispatch_modifier_key_event_before = true;
              }
            } else {
              // key_code, consumer_key_code
              dispatch_modifier_key_event = true;
              dispatch_modifier_key_event_before = true;
            }

          } else if (front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_motion) {
            if (!queue_.get_keyboard_repeat_detector().is_repeating() &&
                pressed_buttons_.empty() &&
                !mouse_key_handler_.active()) {
              dispatch_modifier_key_event = true;
              dispatch_modifier_key_event_before = true;
            }
          }
        }
      }

      if (dispatch_modifier_key_event &&
          dispatch_modifier_key_event_before) {
        key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue->get_modifier_flag_manager(),
                                                          queue_,
                                                          front_input_event.get_event_time_stamp().get_time_stamp());
      }

      switch (front_input_event.get_event().get_type()) {
        case event_queue::queued_event::event::type::key_code:
          if (auto key_code = front_input_event.get_event().get_key_code()) {
            if (auto hid_usage_page = types::make_hid_usage_page(*key_code)) {
              if (auto hid_usage = types::make_hid_usage(*key_code)) {
                if (types::make_modifier_flag(*key_code) == boost::none) {
                  switch (front_input_event.get_event_type()) {
                    case event_type::key_down:
                      key_event_dispatcher_.dispatch_key_down_event(front_input_event.get_device_id(),
                                                                    *hid_usage_page,
                                                                    *hid_usage,
                                                                    queue_,
                                                                    front_input_event.get_event_time_stamp().get_time_stamp());
                      break;

                    case event_type::key_up:
                      key_event_dispatcher_.dispatch_key_up_event(*hid_usage_page,
                                                                  *hid_usage,
                                                                  queue_,
                                                                  front_input_event.get_event_time_stamp().get_time_stamp());
                      break;

                    case event_type::single:
                      break;
                  }
                }
              }
            }
          }
          break;

        case event_queue::queued_event::event::type::consumer_key_code:
          if (auto consumer_key_code = front_input_event.get_event().get_consumer_key_code()) {
            if (auto hid_usage_page = types::make_hid_usage_page(*consumer_key_code)) {
              if (auto hid_usage = types::make_hid_usage(*consumer_key_code)) {
                switch (front_input_event.get_event_type()) {
                  case event_type::key_down:
                    key_event_dispatcher_.dispatch_key_down_event(front_input_event.get_device_id(),
                                                                  *hid_usage_page,
                                                                  *hid_usage,
                                                                  queue_,
                                                                  front_input_event.get_event_time_stamp().get_time_stamp());
                    break;

                  case event_type::key_up:
                    key_event_dispatcher_.dispatch_key_up_event(*hid_usage_page,
                                                                *hid_usage,
                                                                queue_,
                                                                front_input_event.get_event_time_stamp().get_time_stamp());
                    break;

                  case event_type::single:
                    break;
                }
              }
            }
          }
          break;

        case event_queue::queued_event::event::type::pointing_button:
        case event_queue::queued_event::event::type::pointing_motion: {
          pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
          report.buttons = output_event_queue->get_pointing_button_manager().make_hid_report_buttons();

          if (auto pointing_motion = front_input_event.get_event().get_pointing_motion()) {
            report.x = pointing_motion->get_x();
            report.y = pointing_motion->get_y();
            report.vertical_wheel = pointing_motion->get_vertical_wheel();
            report.horizontal_wheel = pointing_motion->get_horizontal_wheel();
          }

          queue_.emplace_back_pointing_input(report,
                                             front_input_event.get_event_type(),
                                             front_input_event.get_event_time_stamp().get_time_stamp());

          // Save buttons for `handle_device_ungrabbed_event`.
          pressed_buttons_ = report.buttons;

          break;
        }

        case event_queue::queued_event::event::type::shell_command:
          if (auto shell_command = front_input_event.get_event().get_shell_command()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              queue_.push_back_shell_command_event(*shell_command,
                                                   front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::queued_event::event::type::select_input_source:
          if (auto input_source_selectors = front_input_event.get_event().get_input_source_selectors()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              queue_.push_back_select_input_source_event(*input_source_selectors,
                                                         front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::queued_event::event::type::mouse_key:
          if (auto mouse_key = front_input_event.get_event().get_mouse_key()) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              mouse_key_handler_.push_back_mouse_key(front_input_event.get_device_id(),
                                                     *mouse_key,
                                                     output_event_queue,
                                                     front_input_event.get_event_time_stamp().get_time_stamp());
            } else {
              mouse_key_handler_.erase_mouse_key(front_input_event.get_device_id(),
                                                 *mouse_key,
                                                 output_event_queue,
                                                 front_input_event.get_event_time_stamp().get_time_stamp());
            }
          }
          break;

        case event_queue::queued_event::event::type::stop_keyboard_repeat:
          if (auto key = queue_.get_keyboard_repeat_detector().get_repeating_key()) {
            key_event_dispatcher_.dispatch_key_up_event(key->first,
                                                        key->second,
                                                        queue_,
                                                        front_input_event.get_event_time_stamp().get_time_stamp());
          }
          break;

        case event_queue::queued_event::event::type::none:
        case event_queue::queued_event::event::type::set_variable:
        case event_queue::queued_event::event::type::device_keys_and_pointing_buttons_are_released:
        case event_queue::queued_event::event::type::device_ungrabbed:
        case event_queue::queued_event::event::type::caps_lock_state_changed:
        case event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
        case event_queue::queued_event::event::type::frontmost_application_changed:
        case event_queue::queued_event::event::type::input_source_changed:
        case event_queue::queued_event::event::type::keyboard_type_changed:
          // Do nothing
          break;
      }

      if (dispatch_modifier_key_event &&
          !dispatch_modifier_key_event_before) {
        key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue->get_modifier_flag_manager(),
                                                          queue_,
                                                          front_input_event.get_event_time_stamp().get_time_stamp());
      }
    }

    return manipulate_result::passed;
  }

  virtual bool active(void) const {
    return !queue_.empty();
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return false;
  }

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::queued_event& front_input_event,
                                                                          event_queue& output_event_queue) {
    // modifier flags

    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      front_input_event.get_event_time_stamp().get_time_stamp());

    // pointing buttons

    {
      auto buttons = output_event_queue.get_pointing_button_manager().make_hid_report_buttons();
      if (pressed_buttons_ != buttons) {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
        report.buttons = buttons;
        queue_.emplace_back_pointing_input(report,
                                           event_type::key_up,
                                           front_input_event.get_event_time_stamp().get_time_stamp());

        // Save buttons for `handle_device_ungrabbed_event`.
        pressed_buttons_ = buttons;
      }
    }

    // mouse keys

    mouse_key_handler_.erase_mouse_keys_by_device_id(front_input_event.get_device_id(),
                                                     front_input_event.get_event_time_stamp().get_time_stamp());
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
    // Release pressed keys

    key_event_dispatcher_.dispatch_key_up_events_by_device_id(device_id,
                                                              queue_,
                                                              time_stamp);

    // Release buttons
    {
      auto buttons = output_event_queue.get_pointing_button_manager().make_hid_report_buttons();
      if (pressed_buttons_ != buttons) {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
        report.buttons = buttons;
        queue_.emplace_back_pointing_input(report,
                                           event_type::key_up,
                                           time_stamp);

        pressed_buttons_ = buttons;
      }
    }

    // Release modifiers

    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      time_stamp);

    // Release mouse_key_handler_

    mouse_key_handler_.erase_mouse_keys_by_device_id(device_id,
                                                     time_stamp);
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    // We should not dispatch modifier key events while key repeating.
    //
    // macOS does not ignore the modifier state change while key repeating.
    // If you enabled `control-f -> right_arrow` configuration,
    // apps will catch control-right_arrow event if release the lazy modifier here while right_arrow key is repeating.

    // We should not dispatch modifier key events while mouse keys active.
    //
    // For example, we should not restore right_shift by physical mouse movement when we use "change right_shift+r to scroll",
    // because the right_shift key_up event interrupt scroll event.

    // We should not dispatch modifier key events while mouse buttons are pressed.
    // If you enabled `option-f -> button1` configuration,
    // apps will catch option-button1 event if release the lazy modifier here while button1 is pressed.

    switch (front_input_event.get_event_type()) {
      case event_type::key_down:
      case event_type::single:
        if (!queue_.get_keyboard_repeat_detector().is_repeating() &&
            pressed_buttons_.empty() &&
            !mouse_key_handler_.active()) {
          key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                            queue_,
                                                            front_input_event.get_event_time_stamp().get_time_stamp());
        }
        break;

      case event_type::key_up:
        break;
    }
  }

  virtual void manipulator_timer_invoked(manipulator_timer::timer_id timer_id, uint64_t now) {
    mouse_key_handler_.manipulator_timer_invoked(timer_id, now);
  }

  virtual void set_valid(bool value) {
    // This manipulator is always valid.
  }

  void post_events(virtual_hid_device_client& virtual_hid_device_client) {
    queue_.post_events(virtual_hid_device_client);
  }

  const queue& get_queue(void) const {
    return queue_;
  }

  void clear_queue(void) {
    return queue_.clear();
  }

  const key_event_dispatcher& get_key_event_dispatcher(void) const {
    return key_event_dispatcher_;
  }

private:
  queue queue_;
  key_event_dispatcher key_event_dispatcher_;
  mouse_key_handler mouse_key_handler_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
  pqrs::karabiner_virtual_hid_device::hid_report::buttons pressed_buttons_;
};

inline std::ostream& operator<<(std::ostream& stream, const post_event_to_virtual_devices::queue::event& event) {
  stream << event.to_json();
  return stream;
}

inline void to_json(nlohmann::json& json, const post_event_to_virtual_devices::queue::event& value) {
  json = value.to_json();
}
} // namespace details
} // namespace manipulator
} // namespace krbn
