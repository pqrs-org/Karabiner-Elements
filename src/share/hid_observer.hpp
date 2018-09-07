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

  boost::signals2::signal<void(void)> device_observed;
  boost::signals2::signal<void(void)> device_unobserved;

  // Methods

  hid_observer(const hid_observer&) = delete;

  hid_observer(std::weak_ptr<human_interface_device> weak_hid) : weak_hid_(weak_hid),
                                                                 observed_(false) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();

    if (auto hid = weak_hid.lock()) {
      // opened
      {
        auto c = hid->opened.connect([this] {
          dispatcher_->enqueue([this] {
            if (auto hid = weak_hid_.lock()) {
              observed_ = true;

              device_observed();

              hid->async_queue_start();
              hid->async_schedule();
            }
          });
        });
        human_interface_device_connections_.push_back(c);
      }

      // open_failed
      {
        auto c = hid->open_failed.connect([this](auto&& error) {
          dispatcher_->enqueue([this, error] {
            if (auto hid = weak_hid_.lock()) {
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
        auto c = hid->closed.connect([this] {
          dispatcher_->enqueue([this] {
            if (auto hid = weak_hid_.lock()) {
              device_unobserved();
            }
          });
        });
        human_interface_device_connections_.push_back(c);
      }

      // close_failed
      {
        auto c = hid->close_failed.connect([this](auto&& error) {
          dispatcher_->enqueue([this, error] {
            if (auto hid = weak_hid_.lock()) {
              auto message = fmt::format("IOHIDDeviceClose error: {0} ({1}) {2}",
                                         iokit_utility::get_error_name(error),
                                         error,
                                         hid->get_name_for_log());
              logger_unique_filter_.error(message);

              device_unobserved();
            }
          });
        });
        human_interface_device_connections_.push_back(c);
      }
    }
  }

  ~hid_observer(void) {
    async_unobserve();

    // Disconnect `human_interface_device_connections_`

    if (auto hid = weak_hid_.lock()) {
      hid->get_run_loop_thread()->enqueue(^{
        human_interface_device_connections_.disconnect_all_connections();
      });
    } else {
      human_interface_device_connections_.disconnect_all_connections();
    }

    human_interface_device_connections_.wait_disconnect_all_connections();

    // Release `dispatcher_`

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  std::weak_ptr<human_interface_device> get_weak_hid(void) {
    return weak_hid_;
  }

  void async_observe(void) {
    dispatcher_->enqueue([this] {
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
          thread_utility::timer::mode::repeat,
          [this] {
            dispatcher_->enqueue([this] {
              if (!observed_) {
                if (auto hid = weak_hid_.lock()) {
                  if (!hid->get_removed()) {
                    hid->async_open();
                    return;
                  }
                }
              }

              timer_->cancel();
            });
          });
    });
  }

  void async_unobserve(void) {
    dispatcher_->enqueue([this] {
      if (!timer_) {
        return;
      }

      timer_->cancel();
      timer_ = nullptr;

      if (observed_) {
        if (auto hid = weak_hid_.lock()) {
          hid->async_unschedule();
          hid->async_queue_stop();
          hid->async_close();
        }

        observed_ = false;
      }
    });
  }

private:
  std::weak_ptr<human_interface_device> weak_hid_;

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  boost_utility::signals2_connections human_interface_device_connections_;
  bool observed_;
  logger::unique_filter logger_unique_filter_;
  std::unique_ptr<thread_utility::timer> timer_;
};
} // namespace krbn
