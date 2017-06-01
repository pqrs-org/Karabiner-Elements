#pragma once

#include "boost_defs.hpp"

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "stream_utility.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include <boost/optional.hpp>
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
      };

      event(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event,
            uint64_t time_stamp) : type_(type::keyboard_event),
                                   keyboard_event_(keyboard_event),
                                   time_stamp_(time_stamp) {
      }

      event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
            uint64_t time_stamp) : type_(type::pointing_input),
                                   pointing_input_(pointing_input),
                                   time_stamp_(time_stamp) {
      }

      type get_type(void) const {
        return type_;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event> get_keyboard_event(void) const {
        if (type_ == type::keyboard_event) {
          return keyboard_event_;
        }
        return boost::none;
      }

      boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::pointing_input> get_pointing_input(void) const {
        if (type_ == type::pointing_input) {
          return pointing_input_;
        }
        return boost::none;
      }

      uint64_t get_time_stamp(void) const {
        return time_stamp_;
      }

      bool operator==(const event& other) const {
        return get_type() == other.get_type() &&
               get_keyboard_event() == other.get_keyboard_event() &&
               get_pointing_input() == other.get_pointing_input() &&
               get_time_stamp() == other.get_time_stamp();
      }

    private:
      type type_;
      union {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event_;
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input_;
      };
      uint64_t time_stamp_;
    };

    queue(void) {
    }

    const std::vector<event>& get_events(void) const {
      return events_;
    }

    void emplace_back_event(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event,
                            uint64_t time_stamp) {
      events_.emplace_back(keyboard_event,
                           time_stamp);
    }

    void emplace_back_event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
                            uint64_t time_stamp) {
      events_.emplace_back(pointing_input,
                           time_stamp);
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

        events_.erase(std::begin(events_));
      }
    }

  private:
    std::vector<event> events_;
    std::unique_ptr<gcd_utility::main_queue_after_timer> timer_;
  };

  class key_event_dispatcher final {
  public:
    typedef std::pair<device_id, key_code> pressed_key;

    struct pressed_key_hash : public std::unary_function<pressed_key, std::size_t> {
      std::size_t operator()(const pressed_key& k) const {
        return static_cast<uint32_t>(k.first) ^
               static_cast<uint32_t>(k.second);
      }
    };

    // Return true if usage_page and usage is not exists before insert. (device_id is ignored.)
    void insert_key(device_id device_id,
                    key_code key_code,
                    queue& queue,
                    uint64_t time_stamp) {
      // Enqueue key_down event if it is not sent yet.

      auto it = std::find_if(std::begin(pressed_keys_),
                             std::end(pressed_keys_),
                             [&](auto& k) {
                               return k.second == key_code;
                             });
      auto found = (it != std::end(pressed_keys_));

      pressed_keys_.emplace(device_id, key_code);

      if (!found) {
        enqueue_key_event(key_code, event_type::key_down, queue, time_stamp);
      }
    }

    // Return true if usage_page and usage is not exists after erase. (device_id is ignored.)
    void erase_key(key_code key_code,
                   queue& queue,
                   uint64_t time_stamp) {
      // Enqueue key_up event if it is already sent.

      bool found = false;
      for (auto it = std::begin(pressed_keys_); it != std::end(pressed_keys_);) {
        if (it->second == key_code) {
          found = true;
          it = pressed_keys_.erase(it);
        } else {
          std::advance(it, 1);
        }
      }

      if (found) {
        enqueue_key_event(key_code, event_type::key_up, queue, time_stamp);
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
            if (auto key_code = types::get_key_code(m)) {
              enqueue_key_event(*key_code, event_type::key_down, queue, time_stamp);
            }
            pressed_modifier_flags_.insert(m);
          }

        } else {
          if (pressed) {
            if (auto key_code = types::get_key_code(m)) {
              enqueue_key_event(*key_code, event_type::key_up, queue, time_stamp);
            }
            pressed_modifier_flags_.erase(m);
          }
        }
      }
    }

    void erase_keys_by_device_id(device_id device_id,
                                 queue& queue,
                                 uint64_t time_stamp) {
      while (true) {
        bool found = false;
        for (const auto& k : pressed_keys_) {
          if (k.first == device_id) {
            found = true;
            erase_key(k.second,
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

    const std::unordered_set<pressed_key, pressed_key_hash>& get_pressed_keys(void) const {
      return pressed_keys_;
    }

  private:
    void enqueue_key_event(key_code key_code,
                           event_type event_type,
                           queue& queue,
                           uint64_t time_stamp) {
      if (auto usage_page = types::get_usage_page(key_code)) {
        if (auto usage = types::get_usage(key_code)) {
          pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
          keyboard_event.usage_page = *usage_page;
          keyboard_event.usage = *usage;
          keyboard_event.value = (event_type == event_type::key_down);
          queue.emplace_back_event(keyboard_event,
                                   time_stamp);
        }
      }
    }

    std::unordered_set<pressed_key, pressed_key_hash> pressed_keys_;
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
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
    output_event_queue.push_back_event(front_input_event);
    front_input_event.set_valid(false);

    if (front_input_event.get_event_type() == event_type::key_down) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }

    switch (front_input_event.get_event().get_type()) {
      case event_queue::queued_event::event::type::key_code:
        if (auto key_code = front_input_event.get_event().get_key_code()) {
          if (types::get_modifier_flag(*key_code) == modifier_flag::zero) {
            if (front_input_event.get_event_type() == event_type::key_down) {
              key_event_dispatcher_.insert_key(front_input_event.get_device_id(),
                                               *key_code,
                                               queue_,
                                               front_input_event.get_time_stamp());
            } else {
              key_event_dispatcher_.erase_key(*key_code,
                                              queue_,
                                              front_input_event.get_time_stamp());
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
            case event_queue::queued_event::event::type::pointing_button:
            case event_queue::queued_event::event::type::device_keys_are_released:
            case event_queue::queued_event::event::type::device_pointing_buttons_are_released:
            case event_queue::queued_event::event::type::device_ungrabbed:
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

      case event_queue::queued_event::event::type::device_keys_are_released:
      case event_queue::queued_event::event::type::device_pointing_buttons_are_released:
      case event_queue::queued_event::event::type::device_ungrabbed:
        // Do nothing
        break;
    }

    if (front_input_event.get_event_type() == event_type::key_up) {
      key_event_dispatcher_.dispatch_modifier_key_event(output_event_queue.get_modifier_flag_manager(),
                                                        queue_,
                                                        front_input_event.get_time_stamp());
    }
  }

  virtual bool active(void) const {
    return !queue_.empty();
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
    // Release pressed keys

    key_event_dispatcher_.erase_keys_by_device_id(device_id,
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
  }

  void post_events(virtual_hid_device_client& virtual_hid_device_client) {
    queue_.post_events(virtual_hid_device_client);
  }

  const queue& get_queue(void) const {
    return queue_;
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

} // namespace details
} // namespace manipulator
} // namespace krbn
