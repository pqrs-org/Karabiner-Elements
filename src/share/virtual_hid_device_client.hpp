#pragma once

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "service_observer.hpp"

class virtual_hid_device_client final {
public:
  typedef std::function<void(virtual_hid_device_client& virtual_hid_device_client)> connected_callback;

  virtual_hid_device_client(const virtual_hid_device_client&) = delete;

  virtual_hid_device_client(spdlog::logger& logger,
                            const connected_callback& connected_callback) : logger_(logger),
                                                                            connected_callback_(connected_callback),
                                                                            service_(IO_OBJECT_NULL),
                                                                            connect_(IO_OBJECT_NULL),
                                                                            virtual_hid_keyboard_initialized_(false),
                                                                            keyboard_type_(krbn::keyboard_type::none) {
    if (auto matching_dictionary = IOServiceNameMatching(pqrs::karabiner_virtual_hid_device::get_virtual_hid_root_name())) {
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

  bool is_virtual_hid_keyboard_initialized(void) const {
    return virtual_hid_keyboard_initialized_;
  }

  void initialize_virtual_hid_keyboard(krbn::keyboard_type keyboard_type) {
    if (virtual_hid_keyboard_initialized_ && keyboard_type_ == keyboard_type) {
      return;
    }

    logger_.info("initialize_virtual_hid_keyboard keyboard_type:{0}", static_cast<int>(keyboard_type));

    keyboard_type_ = keyboard_type;
    virtual_hid_keyboard_initialized_ = call_method([this](void) {
      pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
      properties.keyboard_type = static_cast<pqrs::karabiner_virtual_hid_device::properties::keyboard_type>(keyboard_type_);
      return pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect_, properties);
    });
  }

  void terminate_virtual_hid_keyboard(void) {
    virtual_hid_keyboard_initialized_ = false;

    call_method([this](void) {
      return pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_keyboard(connect_);
    });
  }

  void dispatch_keyboard_event(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event) {
    call_method([this, &keyboard_event](void) {
      return pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect_, keyboard_event);
    });
  }

  void post_keyboard_input_report(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& report) {
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

    virtual_hid_keyboard_initialized_ = false;
  }

  bool call_method(std::function<IOReturn(void)> method) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger_.error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return false;
    }

    auto kr = method();
    if (kr != KERN_SUCCESS) {
      logger_.error("IOConnectCallStructMethod error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      return false;
    }

    return true;
  }

  spdlog::logger& logger_;
  connected_callback connected_callback_;

  std::unique_ptr<service_observer> service_observer_;
  io_service_t service_;
  io_connect_t connect_;
  std::mutex connect_mutex_;

  bool virtual_hid_keyboard_initialized_;
  krbn::keyboard_type keyboard_type_;
};
