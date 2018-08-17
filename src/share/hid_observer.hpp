#pragma once

// `krbn::hid_observer` can be used safely in a multi-threaded environment.

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

  hid_observer(const hid_observer&) = delete;

  hid_observer(std::shared_ptr<human_interface_device> human_interface_device) : human_interface_device_(human_interface_device),
                                                                                 observed_(false) {
    // opened
    {
      auto c = human_interface_device->opened.connect([this] {
        if (auto hid = human_interface_device_.lock()) {
          observed_ = true;

          device_observed(hid);

          hid->async_queue_start();
          hid->async_schedule();
        }
      });
      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }

    // open_failed
    {
      auto c = human_interface_device->open_failed.connect([this](auto&& error) {
        if (auto hid = human_interface_device_.lock()) {
          auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                     iokit_utility::get_error_name(error),
                                     error,
                                     hid->get_name_for_log());
          log_reducer_.error(message);
        }
      });
      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }

    // closed
    {
      auto c = human_interface_device->closed.connect([this] {
        if (auto hid = human_interface_device_.lock()) {
          device_unobserved(hid);
        }
      });
      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }

    // close_failed
    {
      auto c = human_interface_device->close_failed.connect([this](auto&& error) {
        if (auto hid = human_interface_device_.lock()) {
          auto message = fmt::format("IOHIDDeviceClose error: {0} ({1}) {2}",
                                     iokit_utility::get_error_name(error),
                                     error,
                                     hid->get_name_for_log());
          log_reducer_.error(message);

          device_unobserved(hid);
        }
      });
      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }
  }

  ~hid_observer(void) {
    unobserve();

    connections_.clear();
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
          hid->async_unschedule();
          hid->async_queue_stop();
          hid->async_close();
        }

        observed_ = false;
      }
    }
  }

private:
  std::weak_ptr<human_interface_device> human_interface_device_;
  std::vector<std::unique_ptr<boost::signals2::scoped_connection>> connections_;
  std::atomic<bool> observed_;
  spdlog_utility::log_reducer log_reducer_;

  std::unique_ptr<thread_utility::timer> timer_;
  mutable std::mutex timer_mutex_;
};
} // namespace krbn
