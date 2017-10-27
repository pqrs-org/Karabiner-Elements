#pragma once

#include "boost_defs.hpp"

#include "console_user_server_client.hpp"
#include "keyboard_repeat_detector.hpp"
#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "stream_utility.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
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
        keyboard_event,
        pointing_input,
        clear_keyboard_modifier_flags,
        shell_command,
        select_input_source,
      };

      event(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event,
            uint64_t time_stamp) : type_(type::keyboard_event),
                                   value_(keyboard_event),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
            uint64_t time_stamp) : type_(type::pointing_input),
                                   value_(pointing_input),
                                   time_stamp_(time_stamp) {
      }

      static event make_clear_keyboard_modifier_flags_event(uint64_t time_stamp) {
        event e;
        e.type_ = type::clear_keyboard_modifier_flags;
        e.value_ = boost::blank();
        e.time_stamp_ = time_stamp;
        return e;
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

      type get_type(void) const {
        return type_;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event> get_keyboard_event(void) const {
        if (type_ == type::keyboard_event) {
          return boost::get<pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event>(value_);
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

      type type_;
      boost::variant<pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event,
                     pqrs::karabiner_virtual_hid_device::hid_report::pointing_input,
                     boost::blank, // For clear_keyboard_modifier_flags
                     std::string, // For shell_command
                     std::vector<input_source_selector> // For select_input_source
                     >
          value_;
      uint64_t time_stamp_;
    };

    queue(void) : last_event_modifier_key_(false),
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
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
      keyboard_event.usage_page = static_cast<pqrs::karabiner_virtual_hid_device::usage_page>(hid_usage_page);
      keyboard_event.usage = static_cast<pqrs::karabiner_virtual_hid_device::usage>(hid_usage);
      keyboard_event.value = (event_type == event_type::key_down);

      // ----------------------------------------
      // modify time_stamp if needed

      auto m = types::make_modifier_flag(hid_usage_page, hid_usage);
      adjust_time_stamp(time_stamp, m != modifier_flag::zero);

      // ----------------------------------------

      events_.emplace_back(keyboard_event,
                           time_stamp);

      keyboard_repeat_detector_.set(hid_usage_page, hid_usage, event_type);
    }

    void emplace_back_event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
                            uint64_t time_stamp) {
      adjust_time_stamp(time_stamp, false);

      events_.emplace_back(pointing_input,
                           time_stamp);
    }

    void push_back_clear_keyboard_modifier_flags_event(uint64_t time_stamp) {
      // Do not call adjust_time_stamp.
      auto e = event::make_clear_keyboard_modifier_flags_event(time_stamp);
      events_.push_back(e);
    }

    void push_back_shell_command_event(const std::string& shell_command,
                                       uint64_t time_stamp) {
      adjust_time_stamp(time_stamp, false);

      auto e = event::make_shell_command_event(shell_command,
                                               time_stamp);

      events_.push_back(e);
    }

    void push_back_select_input_source_event(const std::vector<input_source_selector>& input_source_selectors,
                                             uint64_t time_stamp) {
      adjust_time_stamp(time_stamp, false);

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

          timer_ = std::make_unique<gcd_utility::main_queue_after_timer>(when, ^{
            post_events(virtual_hid_device_client);
          });
          return;
        }

        if (auto keyboard_event = e.get_keyboard_event()) {
          virtual_hid_device_client.dispatch_keyboard_event(*keyboard_event);
        }
        if (auto pointing_input = e.get_pointing_input()) {
          virtual_hid_device_client.post_pointing_input_report(*pointing_input);
        }
        if (e.get_type() == event::type::clear_keyboard_modifier_flags) {
          virtual_hid_device_client.clear_keyboard_modifier_flags();
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
                           bool is_modifier_key) {
      // Wait is 5 milliseconds
      //
      // Note:
      // * If wait is 1 millisecond, Google Chrome issue below is sometimes happen.
      //

      auto wait = time_utility::nano_to_absolute(5 * NSEC_PER_MSEC);

      if (last_event_modifier_key_ != is_modifier_key &&
          time_stamp < last_event_time_stamp_ + wait) {
        time_stamp = last_event_time_stamp_ + wait;
      }

      last_event_modifier_key_ = is_modifier_key;
      if (last_event_time_stamp_ < time_stamp) {
        last_event_time_stamp_ = time_stamp;
      }
    }

    std::vector<event> events_;
    std::unique_ptr<gcd_utility::main_queue_after_timer> timer_;

    keyboard_repeat_detector keyboard_repeat_detector_;

    // We should add a wait between modifier events and other events in order to
    // ensure window system handles modifier by properly order.
    //
    // Example:
    //
    //   01. left_shift key_down
    //   02. left_control key_down
    //       [wait]
    //   03. a key_down
    //   04. a key_up
    //   05. b key_down
    //   06. b key_up
    //       [wait]
    //   07. left_shift key_up
    //   08. left_control key_up
    //       [wait]
    //   09. button1 down
    //   10. button1 up
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

    bool last_event_modifier_key_;
    uint64_t last_event_time_stamp_;
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
      size_t previous_pressed_modifier_flags_size = pressed_modifier_flags_.size();

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
            if (auto key_code = types::get_key_code(m)) {
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
            if (auto key_code = types::get_key_code(m)) {
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

      if (pressed_modifier_flags_.empty() && previous_pressed_modifier_flags_size > 0) {
        queue.push_back_clear_keyboard_modifier_flags_event(time_stamp);
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

  post_event_to_virtual_devices(void) : base(),
                                        queue_(),
                                        pressed_buttons_(0) {
  }

  virtual ~post_event_to_virtual_devices(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue) {
    output_event_queue.push_back_event(front_input_event);
    front_input_event.set_valid(false);

    // Dispatch modifier key event only when front_input_event is key_down or modifier key.

    bool dispatch_modifier_key_event = false;
    bool dispatch_modifier_key_event_before = false;
    {
      auto modifier_flag = modifier_flag::zero;
      if (auto key_code = front_input_event.get_event().get_key_code()) {
        modifier_flag = types::get_modifier_flag(*key_code);
      }

      if (modifier_flag != modifier_flag::zero) {
        // front_input_event is modifier key event.
        if (!front_input_event.get_lazy()) {
          dispatch_modifier_key_event = true;
          dispatch_modifier_key_event_before = false;
        }

      } else {
        if (front_input_event.get_event_type() == event_type::key_down) {
          if (front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_button) {
            if (!queue_.get_keyboard_repeat_detector().is_repeating()) {
              dispatch_modifier_key_event = true;
              dispatch_modifier_key_event_before = true;
            }
          } else {
            // key_code, consumer_key_code
            dispatch_modifier_key_event = true;
            dispatch_modifier_key_event_before = true;
          }

        } else if (front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_x ||
                   front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_y ||
                   front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_vertical_wheel ||
                   front_input_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_horizontal_wheel) {
          if (!queue_.get_keyboard_repeat_detector().is_repeating()) {
            dispatch_modifier_key_event = true;
            dispatch_modifier_key_event_before = true;
          }
        }
      }
    }

    if (dispatch_modifier_key_event &&
        dispatch_modifier_key_event_before) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }

    switch (front_input_event.get_event().get_type()) {
      case event_queue::queued_event::event::type::key_code:
        if (auto key_code = front_input_event.get_event().get_key_code()) {
          if (auto hid_usage_page = types::make_hid_usage_page(*key_code)) {
            if (auto hid_usage = types::make_hid_usage(*key_code)) {
              if (types::get_modifier_flag(*key_code) == modifier_flag::zero) {
                switch (front_input_event.get_event_type()) {
                  case event_type::key_down:
                    key_event_dispatcher_.dispatch_key_down_event(front_input_event.get_device_id(),
                                                                  *hid_usage_page,
                                                                  *hid_usage,
                                                                  queue_,
                                                                  front_input_event.get_time_stamp());
                    break;

                  case event_type::key_up:
                    key_event_dispatcher_.dispatch_key_up_event(*hid_usage_page,
                                                                *hid_usage,
                                                                queue_,
                                                                front_input_event.get_time_stamp());
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
                                                                front_input_event.get_time_stamp());
                  break;

                case event_type::key_up:
                  key_event_dispatcher_.dispatch_key_up_event(*hid_usage_page,
                                                              *hid_usage,
                                                              queue_,
                                                              front_input_event.get_time_stamp());
                  break;

                case event_type::single:
                  break;
              }
            }
          }
        }
        break;

      case event_queue::queued_event::event::type::pointing_button:
      case event_queue::queued_event::event::type::pointing_x:
      case event_queue::queued_event::event::type::pointing_y:
      case event_queue::queued_event::event::type::pointing_vertical_wheel:
      case event_queue::queued_event::event::type::pointing_horizontal_wheel: {
        auto report = output_event_queue.get_pointing_button_manager().make_pointing_input_report();

        if (auto integer_value = front_input_event.get_event().get_integer_value()) {
          switch (front_input_event.get_event().get_type()) {
            case event_queue::queued_event::event::type::pointing_x:
              report.x = *integer_value;
              break;
            case event_queue::queued_event::event::type::pointing_y:
              report.y = *integer_value;
              break;
            case event_queue::queued_event::event::type::pointing_vertical_wheel:
              report.vertical_wheel = *integer_value;
              break;
            case event_queue::queued_event::event::type::pointing_horizontal_wheel:
              report.horizontal_wheel = *integer_value;
              break;
            case event_queue::queued_event::event::type::key_code:
            case event_queue::queued_event::event::type::consumer_key_code:
            case event_queue::queued_event::event::type::pointing_button:
            case event_queue::queued_event::event::type::set_variable:
            case event_queue::queued_event::event::type::shell_command:
            case event_queue::queued_event::event::type::select_input_source:
            case event_queue::queued_event::event::type::device_keys_are_released:
            case event_queue::queued_event::event::type::device_pointing_buttons_are_released:
            case event_queue::queued_event::event::type::device_ungrabbed:
            case event_queue::queued_event::event::type::caps_lock_state_changed:
            case event_queue::queued_event::event::type::event_from_ignored_device:
            case event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
            case event_queue::queued_event::event::type::frontmost_application_changed:
            case event_queue::queued_event::event::type::input_source_changed:
              // Do nothing
              break;
          }
        }

        queue_.emplace_back_event(report,
                                  front_input_event.get_time_stamp());

        // Save bits for `handle_device_ungrabbed_event`.
        pressed_buttons_ = output_event_queue.get_pointing_button_manager().get_hid_report_bits();

        break;
      }

      case event_queue::queued_event::event::type::shell_command:
        if (auto shell_command = front_input_event.get_event().get_shell_command()) {
          if (front_input_event.get_event_type() == event_type::key_down) {
            queue_.push_back_shell_command_event(*shell_command,
                                                 front_input_event.get_time_stamp());
          }
        }
        break;

      case event_queue::queued_event::event::type::select_input_source:
        if (auto input_source_selectors = front_input_event.get_event().get_input_source_selectors()) {
          if (front_input_event.get_event_type() == event_type::key_down) {
            queue_.push_back_select_input_source_event(*input_source_selectors,
                                                       front_input_event.get_time_stamp());
          }
        }
        break;

      case event_queue::queued_event::event::type::set_variable:
      case event_queue::queued_event::event::type::device_keys_are_released:
      case event_queue::queued_event::event::type::device_pointing_buttons_are_released:
      case event_queue::queued_event::event::type::device_ungrabbed:
      case event_queue::queued_event::event::type::caps_lock_state_changed:
      case event_queue::queued_event::event::type::event_from_ignored_device:
      case event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
      case event_queue::queued_event::event::type::frontmost_application_changed:
      case event_queue::queued_event::event::type::input_source_changed:
        // Do nothing
        break;
    }

    if (dispatch_modifier_key_event &&
        !dispatch_modifier_key_event_before) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }
  }

  virtual bool active(void) const {
    return !queue_.empty();
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return false;
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
      auto bits = output_event_queue.get_pointing_button_manager().get_hid_report_bits();
      if (pressed_buttons_ != bits) {
        auto report = output_event_queue.get_pointing_button_manager().make_pointing_input_report();
        queue_.emplace_back_event(report, time_stamp);

        pressed_buttons_ = bits;
      }
    }

    // Release modifiers

    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      time_stamp);
  }

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
    // We should not dispatch modifier key events while key repeating.
    //
    // macOS does not ignore the modifier state change while key repeating.
    // If you enabled `control-f -> right_arrow` configuration,
    // apps will catch control-right_arrow event if release the lazy modifier here while right_arrow key is repeating.

    if (!queue_.get_keyboard_repeat_detector().is_repeating()) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
    // We should not dispatch modifier key events while key repeating.
    // (See a comment in `handle_event_from_ignored_device`.)

    if (!queue_.get_keyboard_repeat_detector().is_repeating()) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }
  }

  virtual void force_post_modifier_key_event(const event_queue::queued_event& front_input_event,
                                             event_queue& output_event_queue) {
    key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                      queue_,
                                                      front_input_event.get_time_stamp());
  }

  virtual void force_post_pointing_button_event(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
    auto report = output_event_queue.get_pointing_button_manager().make_pointing_input_report();
    queue_.emplace_back_event(report,
                              front_input_event.get_time_stamp());

    // Save bits for `handle_device_ungrabbed_event`.
    pressed_buttons_ = output_event_queue.get_pointing_button_manager().get_hid_report_bits();
  }

  virtual void manipulator_timer_invoked(manipulator_timer::timer_id timer_id) {
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
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
  uint32_t pressed_buttons_;
};

inline std::ostream& operator<<(std::ostream& stream, const post_event_to_virtual_devices::queue::event& event) {
  stream << std::endl
         << "{"
         << "\"type\":";
  stream_utility::output_enum(stream, event.get_type());

  if (auto keyboard_event = event.get_keyboard_event()) {
    stream << ",\"keyboard_event.usage\":" << static_cast<uint32_t>(keyboard_event->usage);
    stream << ",\"keyboard_event.usage_page\":" << static_cast<uint32_t>(keyboard_event->usage_page);
    stream << ",\"keyboard_event.value\":" << static_cast<uint32_t>(keyboard_event->value);
  }

  if (auto pointing_input = event.get_pointing_input()) {
    stream << std::hex;
    stream << ",\"pointing_input.buttons[0]\":0x" << static_cast<int>(pointing_input->buttons[0]);
    stream << ",\"pointing_input.buttons[1]\":0x" << static_cast<int>(pointing_input->buttons[1]);
    stream << ",\"pointing_input.buttons[2]\":0x" << static_cast<int>(pointing_input->buttons[2]);
    stream << ",\"pointing_input.buttons[3]\":0x" << static_cast<int>(pointing_input->buttons[3]);
    stream << std::dec;
    stream << ",\"pointing_input.x\":" << static_cast<int>(pointing_input->x);
    stream << ",\"pointing_input.y\":" << static_cast<int>(pointing_input->y);
    stream << ",\"pointing_input.vertical_wheel\":" << static_cast<int>(pointing_input->vertical_wheel);
    stream << ",\"pointing_input.horizontal_wheel\":" << static_cast<int>(pointing_input->horizontal_wheel);
  }

  stream << ",\"time_stamp\":" << event.get_time_stamp();

  stream << "}";

  return stream;
}
} // namespace details
} // namespace manipulator
} // namespace krbn
