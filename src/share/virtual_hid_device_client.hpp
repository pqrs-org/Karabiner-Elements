#pragma once

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtualhiddevice_methods.hpp"
#include "service_observer.hpp"

class virtual_hid_device_client final {
public:
  typedef std::function<void(virtual_hid_device_client& virtual_hid_device_client)> connected_callback;

  virtual_hid_device_client(const virtual_hid_device_client&) = delete;

  virtual_hid_device_client(spdlog::logger& logger,
                            const connected_callback& connected_callback) : logger_(logger),
                                                                            connected_callback_(connected_callback),
                                                                            service_(IO_OBJECT_NULL),
                                                                            connect_(IO_OBJECT_NULL) {
    if (auto matching_dictionary = IOServiceNameMatching(pqrs::karabiner_virtualhiddevice::get_virtual_hid_root_name())) {
      service_observer_ = std::make_unique<service_observer>(logger_,
                                                             matching_dictionary,
                                                             std::bind(&virtual_hid_device_client::matched_callback, this, std::placeholders::_1),
                                                             std::bind(&virtual_hid_device_client::terminated_callback, this, std::placeholders::_1));
      CFRelease(matching_dictionary);
    }
  }

  ~virtual_hid_device_client(void) {
    close_connection();
  }

  bool is_connected(void) const {
    return connect_ != IO_OBJECT_NULL;
  }

  void initialize_virtual_hid_keyboard(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::initialize_virtual_hid_keyboard(connect_);
    });
  }

  void terminate_virtual_hid_keyboard(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::terminate_virtual_hid_keyboard(connect_);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtualhiddevice_methods::post_keyboard_input_report(connect_, report);
    });
  }

  void reset_virtual_hid_keyboard(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::reset_virtual_hid_keyboard(connect_);
    });
  }

  void initialize_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::initialize_virtual_hid_pointing(connect_);
    });
  }

  void terminate_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::terminate_virtual_hid_pointing(connect_);
    });
  }

  void post_pointing_input_report(const pqrs::karabiner_virtualhiddevice::hid_report::pointing_input& report) {
    call_method([this, &report](void) {
      return pqrs::karabiner_virtualhiddevice_methods::post_pointing_input_report(connect_, report);
    });
  }

  void reset_virtual_hid_pointing(void) {
    call_method([this](void) {
      return pqrs::karabiner_virtualhiddevice_methods::reset_virtual_hid_pointing(connect_);
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
          logger_.error("IOServiceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
          connect_ = IO_OBJECT_NULL;
        }

        logger_.info("IOServiceOpen is succeeded @ {0}", __PRETTY_FUNCTION__);
        connected = true;
      }

      IOObjectRelease(service);
    }

    // We have to call callback after connect_mutex_ is unlocked.
    if (connected && connected_callback_) {
      connected_callback_(*this);
    }
  }

  void terminated_callback(io_iterator_t iterator) {
    while (auto service = IOIteratorNext(iterator)) {
      std::lock_guard<std::mutex> guard(connect_mutex_);

      close_connection();

      IOObjectRelease(service);
    }
  }

  void close_connection(void) {
    if (connect_) {
      auto kr = IOServiceClose(connect_);
      if (kr != kIOReturnSuccess) {
        logger_.error("IOConnectRelease error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      }
      connect_ = IO_OBJECT_NULL;
    }

    logger_.info("IOServiceClose is succeeded @ {0}", __PRETTY_FUNCTION__);

    if (service_) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }
  }

  void call_method(std::function<IOReturn(void)> method) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger_.error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    auto kr = method();
    if (kr != KERN_SUCCESS) {
      logger_.error("IOConnectCallStructMethod error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
    }
  }

  spdlog::logger& logger_;
  connected_callback connected_callback_;

  std::unique_ptr<service_observer> service_observer_;
  io_service_t service_;
  io_connect_t connect_;
  std::mutex connect_mutex_;
};
