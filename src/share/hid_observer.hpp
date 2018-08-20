#pragma once

// `krbn::hid_observer` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"

namespace krbn {
class hid_observer final {
public:
  // Signals

  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_observed;
  boost::signals2::signal<void(std::shared_ptr<human_interface_device>)> device_unobserved;

  // Methods

  hid_observer(const hid_observer&) = delete;

  hid_observer(std::shared_ptr<human_interface_device> human_interface_device) : human_interface_device_(human_interface_device),
                                                                                 observed_(false) {
    queue_ = std::make_unique<thread_utility::queue>();

    // opened
    {
      auto c = human_interface_device->opened.connect([this] {
        queue_->push_back([this] {
          if (auto hid = human_interface_device_.lock()) {
            observed_ = true;

            device_observed(hid);

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
            device_unobserved(hid);
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

            device_unobserved(hid);
          }
        });
      });
      human_interface_device_connections_.push_back(c);
    }
  }

  ~hid_observer(void) {
    async_unobserve();

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

  void async_observe(void) {
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
            return std::chrono::milliseconds(3000);
          }
        },
        true,
        [this] {
          queue_->push_back([this] {
            if (!observed_) {
              if (auto hid = human_interface_device_.lock()) {
                if (!hid->get_removed()) {
                  hid->async_open();
                  return;
                }
              }
            }

            timer_->unset_repeats();
          });
        });
  }

  void async_unobserve(void) {
    {
      std::lock_guard<std::mutex> lock(timer_mutex_);

      if (!timer_) {
        return;
      }

      timer_ = nullptr;
    }

    queue_->push_back([this] {
      if (observed_) {
        if (auto hid = human_interface_device_.lock()) {
          hid->async_unschedule();
          hid->async_queue_stop();
          hid->async_close();
        }

        observed_ = false;
      }
    });
  }

private:
  std::weak_ptr<human_interface_device> human_interface_device_;

  std::unique_ptr<thread_utility::queue> queue_;
  boost_utility::signals2_connections human_interface_device_connections_;
  bool observed_;
  logger::unique_filter logger_unique_filter_;

  std::unique_ptr<thread_utility::timer> timer_;
  mutable std::mutex timer_mutex_;
};
} // namespace krbn
