#pragma once

// `krbn::hid_observers_` can be used safely in a multi-threaded environment.

#include "human_interface_device.hpp"
#include "spdlog_utility.hpp"
#include "thread_utility.hpp"

namespace krbn {
class hid_observer final {
public:
  // Signals

  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_observed;

  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_unobserved;

  // Methods

  hid_observer(std::shared_ptr<human_interface_device> human_interface_device) : human_interface_device_(human_interface_device),
                                                                                 observed_(false) {
    call_slots_queue_ = std::make_unique<thread_utility::queue>();
  }

  ~hid_observer(void) {
    unobserve();

    call_slots_queue_ = nullptr;
  }

  std::weak_ptr<human_interface_device> get_human_interface_device(void) {
    return human_interface_device_;
  }

  void observe(void) {
    std::lock_guard<std::mutex> lock(timer_mutex_);

    if (timer_) {
      return;
    }

    log_reducer_.reset();

    timer_ = std::make_unique<thread_utility::timer>(
        [](auto&& count) {
          if (count == 0) {
            return std::chrono::milliseconds(0);
          } else {
            return std::chrono::milliseconds(3000);
          }
        },
        true,
        [this] {
          if (auto hid = human_interface_device_.lock()) {
            if (hid->get_removed()) {
              timer_->unset_repeats();
              return;
            }

            if (observed_) {
              timer_->unset_repeats();
              return;
            }

            // ----------------------------------------
            auto r = hid->open();
            if (r != kIOReturnSuccess) {
              auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                         iokit_utility::get_error_name(r),
                                         r,
                                         hid->get_name_for_log());
              log_reducer_.error(message);
              // Retry
              return;
            }

            // ----------------------------------------
            observed_ = true;

            call_slots_queue_->push_back([this, hid] {
              device_observed(hid);
            });

            hid->queue_start();
            hid->schedule();
          }

          timer_->unset_repeats();
        });
  }

  void unobserve(void) {
    {
      std::lock_guard<std::mutex> lock(timer_mutex_);

      if (!timer_) {
        return;
      }

      timer_ = nullptr;

      if (observed_) {
        if (auto hid = human_interface_device_.lock()) {
          hid->unschedule();
          hid->queue_stop();
          hid->close();

          call_slots_queue_->push_back([this, hid] {
            device_unobserved(hid);
          });
        }

        observed_ = false;
      }
    }
  }

private:
  std::weak_ptr<human_interface_device> human_interface_device_;
  std::unique_ptr<thread_utility::queue> call_slots_queue_;
  bool observed_;
  spdlog_utility::log_reducer log_reducer_;

  std::unique_ptr<thread_utility::timer> timer_;
  mutable std::mutex timer_mutex_;
};
} // namespace krbn
