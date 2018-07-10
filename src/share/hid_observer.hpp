#pragma once

#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "spdlog_utility.hpp"

namespace krbn {
class hid_observer final {
public:
  // Signals

  boost::signals2::signal<void(human_interface_device&)> device_observed;

  boost::signals2::signal<void(human_interface_device&)> device_unobserved;

  // Methods

  hid_observer(human_interface_device& human_interface_device) : human_interface_device_(human_interface_device),
                                                                 observed_(false) {
  }

  ~hid_observer(void) {
    unobserve();
  }

  bool get_observed(void) const {
    return observed_;
  }

  void observe(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      log_reducer_.reset();

      timer_ = nullptr;

      timer_ = std::make_unique<gcd_utility::fire_while_false_timer>(
          3 * NSEC_PER_SEC,
          ^{
            if (human_interface_device_.get_removed()) {
              return true;
            }

            if (observed_) {
              return true;
            }

            // ----------------------------------------
            auto r = human_interface_device_.open();
            if (r != kIOReturnSuccess) {
              auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                         iokit_utility::get_error_name(r),
                                         r,
                                         human_interface_device_.get_name_for_log());
              log_reducer_.error(message);
              return false;
            }

            // ----------------------------------------
            observed_ = true;

            device_observed(human_interface_device_);

            human_interface_device_.queue_start();
            human_interface_device_.schedule();

            return true;
          });
    });
  }

  void unobserve(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      timer_ = nullptr;

      if (!observed_) {
        return;
      }

      human_interface_device_.unschedule();
      human_interface_device_.queue_stop();
      human_interface_device_.close();

      observed_ = false;

      device_unobserved(human_interface_device_);
    });
  }

private:
  human_interface_device& human_interface_device_;
  bool observed_;
  std::unique_ptr<gcd_utility::fire_while_false_timer> timer_;
  spdlog_utility::log_reducer log_reducer_;
};
} // namespace krbn
