#pragma once

#include "boost_defs.hpp"

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "stream_utility.hpp"
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
          timer_ = std::make_unique<gcd_utility::main_queue_after_timer>(e.get_time_stamp(), ^{
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

  post_event_to_virtual_devices(void) : base(),
                                        queue_() {
  }

  virtual ~post_event_to_virtual_devices(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
    output_event_queue.push_back_event(front_input_event);
    front_input_event.set_valid(false);

    switch (front_input_event.get_event().get_type()) {
      case event_queue::queued_event::event::type::key_code:
        if (auto key_code = front_input_event.get_event().get_key_code()) {
          if (auto usage_page = types::get_usage_page(*key_code)) {
            if (auto usage = types::get_usage(*key_code)) {
              pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
              keyboard_event.usage_page = *usage_page;
              keyboard_event.usage = *usage;
              keyboard_event.value = front_input_event.get_event_type() == event_type::key_down;
              queue_.emplace_back_event(keyboard_event,
                                        front_input_event.get_time_stamp());
            }
          }
          return;
        }
        break;

      case event_queue::queued_event::event::type::pointing_button:
      case event_queue::queued_event::event::type::pointing_x:
      case event_queue::queued_event::event::type::pointing_y:
      case event_queue::queued_event::event::type::pointing_vertical_wheel:
      case event_queue::queued_event::event::type::pointing_horizontal_wheel: {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;

        auto bits = output_event_queue.get_pointing_button_manager().get_hid_report_bits();
        report.buttons[0] = (bits >> 0) & 0xff;
        report.buttons[1] = (bits >> 8) & 0xff;
        report.buttons[2] = (bits >> 16) & 0xff;
        report.buttons[3] = (bits >> 24) & 0xff;

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
              // Do nothing
              break;
          }
        }

        queue_.emplace_back_event(report,
                                  front_input_event.get_time_stamp());
        break;
      }
    }
  }

  virtual bool active(void) const {
    return !queue_.empty();
  }

  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) {
    // Do nothing
  }

  void post_events(virtual_hid_device_client& virtual_hid_device_client) {
    queue_.post_events(virtual_hid_device_client);
  }

  const queue& get_queue(void) const {
    return queue_;
  }

private:
  queue queue_;
};

} // namespace details
} // namespace manipulator
} // namespace krbn
