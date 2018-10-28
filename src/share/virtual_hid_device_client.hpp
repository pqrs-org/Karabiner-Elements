#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "iokit_utility.hpp"
#include "monitor/service_monitor.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class virtual_hid_device_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  boost::signals2::signal<void(void)> client_connected;
  boost::signals2::signal<void(void)> client_disconnected;
  boost::signals2::signal<void(void)> virtual_hid_keyboard_ready;

  virtual_hid_device_client(const virtual_hid_device_client&) = delete;

  virtual_hid_device_client(void) : dispatcher_client(),
                                    service_(IO_OBJECT_NULL),
                                    connect_(IO_OBJECT_NULL),
                                    connected_(false),
                                    virtual_hid_keyboard_ready_check_timer_(*this),
                                    virtual_hid_keyboard_ready_(false) {
  }

  virtual ~virtual_hid_device_client(void) {
    detach_from_dispatcher([this] {
      close_connection();

      service_monitor_ = nullptr;
    });
  }

  void async_connect(void) {
    enqueue_to_dispatcher([this] {
      if (auto matching_dictionary = IOServiceNameMatching(pqrs::karabiner_virtual_hid_device::get_virtual_hid_root_name())) {
        service_monitor_ = std::make_unique<monitor::service_monitor::service_monitor>(matching_dictionary);

        service_monitor_->service_detected.connect([this](auto&& services) {
          if (!services->get_services().empty()) {
            close_connection();

            // Use first matched service.
            open_connection(services->get_services().front());
          }
        });

        service_monitor_->service_removed.connect([this](auto&& services) {
          if (!services->get_services().empty()) {
            close_connection();

            // Use the next service
            service_monitor_->async_invoke_service_detected();
          }
        });

        service_monitor_->async_start();

        logger::get_logger().info("virtual_hid_device_client is started.");

        CFRelease(matching_dictionary);
      }
    });
  }

  void async_close(void) {
    enqueue_to_dispatcher([this] {
      close_connection();
    });
  }

  bool is_connected(void) const {
    return connected_;
  }

  bool is_virtual_hid_keyboard_ready(void) const {
    return virtual_hid_keyboard_ready_;
  }

  void async_initialize_virtual_hid_keyboard(const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization& properties) {
    enqueue_to_dispatcher([this, properties] {
      if (virtual_hid_keyboard_ready_ &&
          virtual_hid_keyboard_properties_ == properties) {
        return;
      }

      virtual_hid_keyboard_properties_ = properties;
      virtual_hid_keyboard_ready_ = false;

      logger::get_logger().info("initialize_virtual_hid_keyboard");
      logger::get_logger().info("  country_code:{0}", static_cast<uint32_t>(virtual_hid_keyboard_properties_.country_code));

      if (!connect_) {
        logger::get_logger().warn("connect_ is IO_OBJECT_NULL");
        return;
      }

      bool result = pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect_, virtual_hid_keyboard_properties_);

      if (result == kIOReturnSuccess) {
        virtual_hid_keyboard_ready_check_timer_.start(
            [this] {
              if (!connect_) {
                virtual_hid_keyboard_ready_check_timer_.stop();
                return;
              }

              bool ready = false;
              if (pqrs::karabiner_virtual_hid_device_methods::is_virtual_hid_keyboard_ready(connect_, ready) == kIOReturnSuccess) {
                if (ready) {
                  virtual_hid_keyboard_ready_check_timer_.stop();
                  virtual_hid_keyboard_ready_ = true;

                  enqueue_to_dispatcher([this] {
                    virtual_hid_keyboard_ready();
                  });
                }
              }
            },
            std::chrono::milliseconds(1000));
      }
    });
  }

  void async_terminate_virtual_hid_keyboard(void) {
    enqueue_to_dispatcher([this] {
      virtual_hid_keyboard_ready_check_timer_.stop();
      virtual_hid_keyboard_ready_ = false;

      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_keyboard(connect_);
      }
    });
  }

  void async_post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& report) {
    enqueue_to_dispatcher([this, report] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
      }
    });
  }

  void async_post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::consumer_input& report) {
    enqueue_to_dispatcher([this, report] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
      }
    });
  }

  void async_post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input& report) {
    enqueue_to_dispatcher([this, report] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
      }
    });
  }

  void async_post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input& report) {
    enqueue_to_dispatcher([this, report] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
      }
    });
  }

  void async_reset_virtual_hid_keyboard(void) {
    enqueue_to_dispatcher([this] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_keyboard(connect_);
      }
    });
  }

  void async_initialize_virtual_hid_pointing(void) {
    enqueue_to_dispatcher([this] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_pointing(connect_);
      }
    });
  }

  void async_terminate_virtual_hid_pointing(void) {
    enqueue_to_dispatcher([this] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_pointing(connect_);
      }
    });
  }

  void async_post_pointing_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& report) {
    enqueue_to_dispatcher([this, report] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::post_pointing_input_report(connect_, report);
      }
    });
  }

  void async_reset_virtual_hid_pointing(void) {
    enqueue_to_dispatcher([this] {
      if (connect_) {
        pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_pointing(connect_);
      }
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void open_connection(io_service_t s) {
    service_ = s;
    IOObjectRetain(service_);

    auto kr = IOServiceOpen(service_, mach_task_self(), kIOHIDServerConnectType, &connect_);
    if (kr == KERN_SUCCESS) {
      logger::get_logger().info("virtual_hid_device_client is opened.");

      connected_ = true;

      enqueue_to_dispatcher([this] {
        client_connected();
      });

    } else {
      logger::get_logger().error("virtual_hid_device_client::open_connection is failed: {0}",
                                 iokit_utility::get_error_name(kr));
      connect_ = IO_OBJECT_NULL;
    }
  }

  // This method is executed in the dispatcher thread.
  void close_connection(void) {
    if (connect_) {
      auto kr = IOServiceClose(connect_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("virtual_hid_device_client::close_connection error: {0}",
                                   iokit_utility::get_error_name(kr));
      }
      connect_ = IO_OBJECT_NULL;

      connected_ = false;

      enqueue_to_dispatcher([this] {
        client_disconnected();
      });

      logger::get_logger().info("virtual_hid_device_client is closed.");
    }

    if (service_) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }

    virtual_hid_keyboard_ready_check_timer_.stop();
    virtual_hid_keyboard_ready_ = false;
  }

  std::unique_ptr<monitor::service_monitor::service_monitor> service_monitor_;
  io_service_t service_;
  io_connect_t connect_;
  std::atomic<bool> connected_;

  pqrs::dispatcher::extra::timer virtual_hid_keyboard_ready_check_timer_;
  std::atomic<bool> virtual_hid_keyboard_ready_;
  pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization virtual_hid_keyboard_properties_;
};
} // namespace krbn
