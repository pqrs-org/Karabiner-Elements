#pragma once

// `krbn::hid_observer` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "dispatcher.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"

namespace krbn {
class hid_observer final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(void)> device_observed;
  boost::signals2::signal<void(void)> device_unobserved;

  // Methods

  hid_observer(const hid_observer&) = delete;

  hid_observer(std::weak_ptr<human_interface_device> weak_hid) : dispatcher_client(),
                                                                 weak_hid_(weak_hid),
                                                                 timer_(*this),
                                                                 observed_(false) {
    if (auto hid = weak_hid.lock()) {
      // opened
      {
        auto c = hid->opened.connect([this] {
          if (auto hid = weak_hid_.lock()) {
            observed_ = true;

            enqueue_to_dispatcher([this] {
              device_observed();
            });

            hid->async_queue_start();
            hid->async_schedule();
          }
        });
        human_interface_device_connections_.push_back(c);
      }

      // open_failed
      {
        auto c = hid->open_failed.connect([this](auto&& error) {
          if (auto hid = weak_hid_.lock()) {
            auto message = fmt::format("IOHIDDeviceOpen error: {0} ({1}) {2}",
                                       iokit_utility::get_error_name(error),
                                       error,
                                       hid->get_name_for_log());
            logger_unique_filter_.error(message);
          }
        });
        human_interface_device_connections_.push_back(c);
      }

      // closed
      {
        auto c = hid->closed.connect([this] {
          if (auto hid = weak_hid_.lock()) {
            enqueue_to_dispatcher([this] {
              device_unobserved();
            });
          }
        });
        human_interface_device_connections_.push_back(c);
      }

      // close_failed
      {
        auto c = hid->close_failed.connect([this](auto&& error) {
          if (auto hid = weak_hid_.lock()) {
            auto message = fmt::format("IOHIDDeviceClose error: {0} ({1}) {2}",
                                       iokit_utility::get_error_name(error),
                                       error,
                                       hid->get_name_for_log());
            logger_unique_filter_.error(message);

            enqueue_to_dispatcher([this] {
              device_unobserved();
            });
          }
        });
        human_interface_device_connections_.push_back(c);
      }
    }
  }

  virtual ~hid_observer(void) {
    detach_from_dispatcher([this] {
      unobserve();
      human_interface_device_connections_.disconnect_all_connections();
    });
  }

  std::weak_ptr<human_interface_device> get_weak_hid(void) {
    return weak_hid_;
  }

  void async_observe(void) {
    enqueue_to_dispatcher([this] {
      logger_unique_filter_.reset();
    });

    timer_.start(
        [this] {
          observe();
        },
        std::chrono::milliseconds(3000));
  }

  void async_unobserve(void) {
    timer_.stop();

    enqueue_to_dispatcher([this] {
      unobserve();
    });
  }

private:
  void observe(void) {
    if (observed_) {
      timer_.stop();
      return;
    }

    if (auto hid = weak_hid_.lock()) {
      if (!hid->get_removed()) {
        hid->async_open();
      }
    }
  }

  void unobserve(void) {
    if (observed_) {
      if (auto hid = weak_hid_.lock()) {
        hid->async_unschedule();
        hid->async_queue_stop();
        hid->async_close();
      }

      observed_ = false;
    }
  }

  std::weak_ptr<human_interface_device> weak_hid_;

  boost_utility::signals2_connections human_interface_device_connections_;
  pqrs::dispatcher::extra::timer timer_;
  bool observed_;
  logger::unique_filter logger_unique_filter_;
};
} // namespace krbn
