#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "virtual_hid_device_client.hpp"
#include <mach/mach_time.h>

namespace krbn {
namespace manipulator {
namespace details {
class post_event_to_virtual_devices final : public base {
public:
  post_event_to_virtual_devices(void) : base() {
  }

  virtual ~post_event_to_virtual_devices(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
  }

  virtual bool active(void) const {
    return false;
  }

  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) {
  }

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

      const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event* get_keyboard_event(void) const {
        if (type_ == type::keyboard_event) {
          return &keyboard_event_;
        }
        return nullptr;
      }

      const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input* get_pointing_input(void) const {
        if (type_ == type::pointing_input) {
          return &pointing_input_;
        }
        return nullptr;
      }

      uint64_t get_time_stamp(void) const {
        return time_stamp_;
      }

    private:
      type type_;
      union {
        pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event_;
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input pointing_input_;
      };
      uint64_t time_stamp_;
    };

    queue(virtual_hid_device_client& virtual_hid_device_client) : virtual_hid_device_client_(virtual_hid_device_client) {
    }

    void emplace_back_event(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event,
                            uint64_t time_stamp) {
      events_.emplace_back(keyboard_event,
                           time_stamp);
      post_events();
    }

    void emplace_back_event(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& pointing_input,
                            uint64_t time_stamp) {
      events_.emplace_back(pointing_input,
                           time_stamp);
      post_events();
    }

  private:
    void post_events(void) {
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
            post_events();
          });
          return;
        }

        if (auto keyboard_event = e.get_keyboard_event()) {
          virtual_hid_device_client_.dispatch_keyboard_event(*keyboard_event);
        }
        if (auto pointing_input = e.get_pointing_input()) {
          virtual_hid_device_client_.post_pointing_input_report(*pointing_input);
        }

        events_.erase(std::begin(events_));
      }
    }

    virtual_hid_device_client& virtual_hid_device_client_;

    std::vector<event> events_;
    std::unique_ptr<gcd_utility::main_queue_after_timer> timer_;
  };
};
} // namespace details
} // namespace manipulator
} // namespace krbn
