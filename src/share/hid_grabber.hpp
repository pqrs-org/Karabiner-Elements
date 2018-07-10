#pragma once

#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "spdlog_utility.hpp"

namespace krbn {
class hid_grabber final {
public:
  struct signal2_combiner_call_while_grabbable {
    typedef grabbable_state result_type;

    template <typename input_iterator>
    result_type operator()(input_iterator first_observer,
                           input_iterator last_observer) const {
      result_type value = grabbable_state::grabbable;
      for (;
           first_observer != last_observer && value == grabbable_state::grabbable;
           std::advance(first_observer, 1)) {
        value = *first_observer;
      }
      return value;
    }
  };

  // Signals

  boost::signals2::signal<grabbable_state(human_interface_device&),
                          signal2_combiner_call_while_grabbable>
      device_grabbing;

  boost::signals2::signal<void(human_interface_device&)> device_grabbed;

  boost::signals2::signal<void(human_interface_device&)> device_ungrabbed;

  // Methods

  hid_grabber(human_interface_device& human_interface_device) : human_interface_device_(human_interface_device),
                                                                grabbed_(false) {
  }

  bool get_grabbed(void) const {
    return grabbed_;
  }

  void grab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      log_reducer_.reset();

      timer_ = nullptr;

      timer_ = std::make_unique<gcd_utility::fire_while_false_timer>(
          1 * NSEC_PER_SEC,
          ^{
            if (human_interface_device_.get_removed()) {
              return true;
            }

            if (grabbed_) {
              return true;
            }

            switch (device_grabbing(human_interface_device_)) {
              case grabbable_state::grabbable:
                break;

              case grabbable_state::ungrabbable_temporarily:
              case grabbable_state::device_error:
                return false;

              case grabbable_state::ungrabbable_permanently:
                return true;
            }

            // ----------------------------------------
            auto r = human_interface_device_.open(kIOHIDOptionsTypeSeizeDevice);
            if (r != kIOReturnSuccess) {
              auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                         iokit_utility::get_error_name(r),
                                         r,
                                         human_interface_device_.get_name_for_log());
              log_reducer_.error(message);
              return false;
            }

            // ----------------------------------------
            grabbed_ = true;

            device_grabbed(human_interface_device_);

            human_interface_device_.queue_start();
            human_interface_device_.schedule();

            return true;
          });
    });
  }

  void ungrab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      timer_ = nullptr;

      if (!grabbed_) {
        return;
      }

      human_interface_device_.unschedule();
      human_interface_device_.queue_stop();
      human_interface_device_.close();

      grabbed_ = false;

      device_ungrabbed(human_interface_device_);
    });
  }

private:
  human_interface_device& human_interface_device_;
  bool grabbed_;
  std::unique_ptr<gcd_utility::fire_while_false_timer> timer_;
  spdlog_utility::log_reducer log_reducer_;
};
} // namespace krbn
