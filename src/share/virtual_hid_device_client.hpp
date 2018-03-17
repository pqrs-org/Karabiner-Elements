#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "iokit_utility.hpp"
#include "service_observer.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class virtual_hid_device_client final {
public:
  boost::signals2::signal<void(void)> client_connected;
  boost::signals2::signal<void(void)> client_disconnected;
  boost::signals2::signal<void(void)> virtual_hid_keyboard_ready;

  virtual_hid_device_client(const virtual_hid_device_client&) = delete;

  virtual_hid_device_client(void) : service_(IO_OBJECT_NULL),
                                    connect_(IO_OBJECT_NULL),
                                    virtual_hid_keyboard_ready_(false) {
  }

  ~virtual_hid_device_client(void) {
    close_connection();
  }

  void connect(void) {
    if (auto matching_dictionary = IOServiceNameMatching(pqrs::karabiner_virtual_hid_device::get_virtual_hid_root_name())) {
      service_observer_ = std::make_unique<service_observer>(matching_dictionary,
                                                             std::bind(&virtual_hid_device_client::matched_callback, this, std::placeholders::_1),
                                                             std::bind(&virtual_hid_device_client::terminated_callback, this, std::placeholders::_1));
      CFRelease(matching_dictionary);
    }
  }

  void close(void) {
    close_connection();
  }

  bool is_connected(void) const {
    return connect_ != IO_OBJECT_NULL;
  }

  bool is_virtual_hid_keyboard_ready(void) const {
    return virtual_hid_keyboard_ready_;
  }

  void initialize_virtual_hid_keyboard(const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization& properties) {
    if (virtual_hid_keyboard_ready_ &&
        virtual_hid_keyboard_properties_ == properties) {
      return;
    }

    virtual_hid_keyboard_properties_ = properties;
    virtual_hid_keyboard_ready_ = false;

    call_method([this](void) {
      logger::get_logger().info("initialize_virtual_hid_keyboard");
      logger::get_logger().info("  country_code:{0}", static_cast<uint32_t>(virtual_hid_keyboard_properties_.country_code));
      logger::get_logger().info("  caps_lock_delay_milliseconds:{0}", static_cast<uint64_t>(virtual_hid_keyboard_properties_.caps_lock_delay_milliseconds));

      bool result = pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect_, virtual_hid_keyboard_properties_);

      if (result == kIOReturnSuccess) {
        virtual_hid_keyboard_ready_check_timer_ = std::make_unique<gcd_utility::main_queue_timer>(
            dispatch_time(DISPATCH_TIME_NOW, 0 * NSEC_PER_SEC),
            1.0 * NSEC_PER_SEC,
            0,
            ^{
              if (!connect_) {
                virtual_hid_keyboard_ready_check_timer_ = nullptr;
                return;
              }

              bool ready = false;
              if (pqrs::karabiner_virtual_hid_device_methods::is_virtual_hid_keyboard_ready(connect_, ready) == kIOReturnSuccess) {
                if (ready) {
                  virtual_hid_keyboard_ready_check_timer_ = nullptr;
                  virtual_hid_keyboard_ready_ = true;
                  virtual_hid_keyboard_ready();
                }
              }
            });
      }

      return result;
    });
  }

  void terminate_virtual_hid_keyboard(void) {
    virtual_hid_keyboard_ready_check_timer_ = nullptr;
    virtual_hid_keyboard_ready_ = false;

    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_keyboard(connect_);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::consumer_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect_, report);
    });
  }

  void reset_virtual_hid_keyboard(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_keyboard(connect_);
    });
  }

  void initialize_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_pointing(connect_);
    });
  }

  void terminate_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_pointing(connect_);
    });
  }

  void post_pointing_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtual_hid_device_methods::post_pointing_input_report(connect_, report);
    });
  }

  void reset_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_pointing(connect_);
    });
  }

private:
  void matched_callback(io_iterator_t iterator) {
    bool connected = false;

    while (auto service = IOIteratorNext(iterator)) {
      std::lock_guard<std::mutex> guard(connect_mutex_);

      // Use first matched service.
      if (!service_) {
        service_ = service;
        IOObjectRetain(service_);

        auto kr = IOServiceOpen(service_, mach_task_self(), kIOHIDServerConnectType, &connect_);
        if (kr != KERN_SUCCESS) {
          logger::get_logger().error("IOServiceOpen error: {1} ({2}) @ {0}", __PRETTY_FUNCTION__, iokit_utility::get_error_name(kr), kr);
          connect_ = IO_OBJECT_NULL;
        } else {
          logger::get_logger().info("IOServiceOpen is succeeded @ {0}", __PRETTY_FUNCTION__);
          connected = true;
        }
      }

      IOObjectRelease(service);
    }

    // We have to call callbacks after connect_mutex_ is unlocked.
    if (connected) {
      client_connected();
    }
  }

  void terminated_callback(io_iterator_t iterator) {
    bool disconnected = false;

    while (auto service = IOIteratorNext(iterator)) {
      std::lock_guard<std::mutex> guard(connect_mutex_);

      close_connection();

      disconnected = true;

      IOObjectRelease(service);
    }

    // We have to call callbacks after connect_mutex_ is unlocked.
    if (disconnected) {
      client_disconnected();
    }
  }

  void close_connection(void) {
    logger::get_logger().info("virtual_hid_device_client::close_connection");

    if (connect_) {
      auto kr = IOServiceClose(connect_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IOConnectRelease error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      }
      connect_ = IO_OBJECT_NULL;
    }

    if (service_) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }

    virtual_hid_keyboard_ready_check_timer_ = nullptr;
    virtual_hid_keyboard_ready_ = false;
  }

  bool call_method(std::function<IOReturn(void)> method) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger::get_logger().error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return false;
    }

    auto kr = method();
    if (kr != KERN_SUCCESS) {
      if (virtual_hid_keyboard_ready_) {
        logger::get_logger().error("IOConnectCallStructMethod error: {1} ({2}) @ {0}", __PRETTY_FUNCTION__, iokit_utility::get_error_name(kr), kr);
      }
      return false;
    }

    return true;
  }

  std::unique_ptr<service_observer> service_observer_;
  io_service_t service_;
  io_connect_t connect_;
  std::mutex connect_mutex_;

  std::unique_ptr<gcd_utility::main_queue_timer> virtual_hid_keyboard_ready_check_timer_;
  bool virtual_hid_keyboard_ready_;
  pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization virtual_hid_keyboard_properties_;
};
} // namespace krbn
