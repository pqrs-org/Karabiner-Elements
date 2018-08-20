#pragma once

// `krbn::hid_grabber` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"

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

  hid_grabber(const hid_grabber&) = delete;

  hid_grabber(std::shared_ptr<human_interface_device> human_interface_device) : human_interface_device_(human_interface_device),
                                                                                grabbed_(false) {
    queue_ = std::make_unique<thread_utility::queue>();

    // opened
    {
      auto c = human_interface_device->opened.connect([this] {
        queue_->push_back([this] {
          if (auto hid = human_interface_device_.lock()) {
            grabbed_ = true;

            device_grabbed(hid);

            hid->async_queue_start();
            hid->async_schedule();
          }
        });
      });
      human_interface_device_connections_.push_back(c);
    }

    // open_failed
    {
      auto c = human_interface_device->open_failed.connect([this](auto&& error) {
        queue_->push_back([this, error] {
          if (auto hid = human_interface_device_.lock()) {
            auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                       iokit_utility::get_error_name(error),
                                       error,
                                       hid->get_name_for_log());
            logger_unique_filter_.error(message);
          }
        });
      });
      human_interface_device_connections_.push_back(c);
    }

    // closed
    {
      auto c = human_interface_device->closed.connect([this] {
        queue_->push_back([this] {
          if (auto hid = human_interface_device_.lock()) {
            device_ungrabbed(hid);
          }
        });
      });
      human_interface_device_connections_.push_back(c);
    }

    // close_failed
    {
      auto c = human_interface_device->close_failed.connect([this](auto&& error) {
        queue_->push_back([this, error] {
          if (auto hid = human_interface_device_.lock()) {
            auto message = fmt::format("IOHIDDeviceClose error: {0} ({1}) {2}",
                                       iokit_utility::get_error_name(error),
                                       error,
                                       hid->get_name_for_log());
            logger_unique_filter_.error(message);

            device_ungrabbed(hid);
          }
        });
      });
      human_interface_device_connections_.push_back(c);
    }
  }

  ~hid_grabber(void) {
    async_ungrab();

    // Disconnect `human_interface_device_connections_`

    if (auto hid = human_interface_device_.lock()) {
      hid->get_run_loop_thread()->enqueue(^{
        human_interface_device_connections_.disconnect_all_connections();
      });
    } else {
      human_interface_device_connections_.disconnect_all_connections();
    }

    human_interface_device_connections_.wait_disconnect_all_connections();

    // Release `queue_`

    queue_ = nullptr;
  }

  std::weak_ptr<human_interface_device> get_human_interface_device(void) {
    return human_interface_device_;
  }

  grabbable_state::state make_grabbable_state(void) {
    if (auto hid = human_interface_device_.lock()) {
      return device_grabbing(hid);
    }

    return grabbable_state::state::ungrabbable_permanently;
  }

  void async_grab(void) {
    std::lock_guard<std::mutex> lock(timer_mutex_);

    if (timer_) {
      return;
    }

    logger_unique_filter_.reset();

    timer_ = std::make_unique<thread_utility::timer>(
        [](auto&& count) {
          if (count == 0) {
            return std::chrono::milliseconds(0);
          } else {
            return std::chrono::milliseconds(1000);
          }
        },
        true,
        [this] {
          queue_->push_back([this] {
            if (!grabbed_) {
              if (auto hid = human_interface_device_.lock()) {
                if (!hid->get_removed()) {
                  switch (make_grabbable_state()) {
                    case grabbable_state::state::grabbable:
                      hid->async_open(kIOHIDOptionsTypeSeizeDevice);
                      return;

                    case grabbable_state::state::none:
                    case grabbable_state::state::ungrabbable_temporarily:
                    case grabbable_state::state::device_error:
                      // Retry
                      return;

                    case grabbable_state::state::ungrabbable_permanently:
                      break;
                  }
                }
              }
            }

            timer_->unset_repeats();
          });
        });
  }

  void async_ungrab(void) {
    {
      std::lock_guard<std::mutex> lock(timer_mutex_);

      if (!timer_) {
        return;
      }

      timer_ = nullptr;
    }

    queue_->push_back([this] {
      if (grabbed_) {
        if (auto hid = human_interface_device_.lock()) {
          hid->async_unschedule();
          hid->async_queue_stop();
          hid->async_close();
        }

        grabbed_ = false;
      }
    });
  }

private:
  std::weak_ptr<human_interface_device> human_interface_device_;

  std::unique_ptr<thread_utility::queue> queue_;
  boost_utility::signals2_connections human_interface_device_connections_;
  bool grabbed_;
  logger::unique_filter logger_unique_filter_;

  std::unique_ptr<thread_utility::timer> timer_;
  mutable std::mutex timer_mutex_;
};
} // namespace krbn
