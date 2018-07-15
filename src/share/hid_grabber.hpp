#pragma once

#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "spdlog_utility.hpp"

namespace krbn {
class hid_grabber final {
public:
  struct signal2_combiner_call_while_grabbable {
    typedef grabbable_state::state result_type;

    template <typename input_iterator>
    result_type operator()(input_iterator first_observer,
                           input_iterator last_observer) const {
      result_type value = grabbable_state::state::grabbable;
      for (;
           first_observer != last_observer && value == grabbable_state::state::grabbable;
           std::advance(first_observer, 1)) {
        value = *first_observer;
      }
      return value;
    }
  };

  // Signals

  boost::signals2::signal<grabbable_state::state(std::shared_ptr<human_interface_device>),
                          signal2_combiner_call_while_grabbable>
      device_grabbing;

  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_grabbed;

  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_ungrabbed;

  // Methods

  hid_grabber(std::shared_ptr<human_interface_device> human_interface_device) : human_interface_device_(human_interface_device),
                                                                                grabbed_(false) {
  }

  ~hid_grabber(void) {
    ungrab();
  }

  std::weak_ptr<human_interface_device> get_human_interface_device(void) {
    return human_interface_device_;
  }

  bool get_grabbed(void) const {
    return grabbed_;
  }

  grabbable_state::state make_grabbable_state(void) {
    if (auto hid = human_interface_device_.lock()) {
      return device_grabbing(hid);
    }

    return grabbable_state::state::ungrabbable_permanently;
  }

  void grab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      log_reducer_.reset();

      timer_ = nullptr;

      timer_ = std::make_unique<gcd_utility::fire_while_false_timer>(
          1 * NSEC_PER_SEC,
          ^{
            if (auto hid = human_interface_device_.lock()) {
              if (hid->get_removed()) {
                return true;
              }

              if (grabbed_) {
                return true;
              }

              switch (make_grabbable_state()) {
                case grabbable_state::state::grabbable:
                  break;

                case grabbable_state::state::none:
                case grabbable_state::state::ungrabbable_temporarily:
                case grabbable_state::state::device_error:
                  // Retry
                  return false;

                case grabbable_state::state::ungrabbable_permanently:
                  return true;
              }

              // ----------------------------------------
              auto r = hid->open(kIOHIDOptionsTypeSeizeDevice);
              if (r != kIOReturnSuccess) {
                auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                           iokit_utility::get_error_name(r),
                                           r,
                                           hid->get_name_for_log());
                log_reducer_.error(message);
                return false;
              }

              // ----------------------------------------
              grabbed_ = true;

              device_grabbed(hid);

              hid->queue_start();
              hid->schedule();
            }

            return true;
          });
    });
  }

  void ungrab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      timer_ = nullptr;

      if (auto hid = human_interface_device_.lock()) {
        if (!grabbed_) {
          return;
        }

        hid->unschedule();
        hid->queue_stop();
        hid->close();

        grabbed_ = false;

        device_ungrabbed(hid);
      }
    });
  }

private:
  std::weak_ptr<human_interface_device> human_interface_device_;
  bool grabbed_;
  std::unique_ptr<gcd_utility::fire_while_false_timer> timer_;
  spdlog_utility::log_reducer log_reducer_;
};
} // namespace krbn
