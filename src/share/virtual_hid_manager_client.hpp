#pragma once

#include "hid_report.hpp"
#include "service_observer.hpp"
#include "virtual_hid_manager_user_client_method.hpp"

class virtual_hid_manager_client final {
public:
  virtual_hid_manager_client(const virtual_hid_manager_client&) = delete;

  virtual_hid_manager_client(spdlog::logger& logger) : logger_(logger),
                                                       service_(IO_OBJECT_NULL),
                                                       connect_(IO_OBJECT_NULL) {
    if (auto matching_dictionary = IOServiceNameMatching("org_pqrs_driver_VirtualHIDManager")) {
      service_observer_ = std::make_unique<service_observer>(logger_,
                                                             matching_dictionary,
                                                             std::bind(&virtual_hid_manager_client::matched_callback, this, std::placeholders::_1),
                                                             std::bind(&virtual_hid_manager_client::terminated_callback, this, std::placeholders::_1));
      CFRelease(matching_dictionary);
    }
  }

  ~virtual_hid_manager_client(void) {
    close_connection();
  }

  bool is_connected(void) const {
    return connect_ != IO_OBJECT_NULL;
  }

  void call_struct_method(uint32_t selector,
                          const void* _Nullable input_struct,
                          size_t input_struct_length,
                          void* _Nullable output_struct,
                          size_t* _Nullable output_struct_length) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger_.error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    auto kr = IOConnectCallStructMethod(connect_, selector, input_struct, input_struct_length, output_struct, output_struct_length);
    if (kr != KERN_SUCCESS) {
      logger_.error("IOConnectCallStructMethod error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
    }
  }

  void post_keyboard_input_report(const hid_report::keyboard_input& report) {
    call_struct_method(static_cast<uint32_t>(virtual_hid_manager_user_client_method::keyboard_input_report),
                       static_cast<const void*>(&report), sizeof(report),
                       nullptr, 0);
  }

private:
  void matched_callback(io_iterator_t iterator) {
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
      }

      IOObjectRelease(service);
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

  spdlog::logger& logger_;

  std::unique_ptr<service_observer> service_observer_;
  io_service_t service_;
  io_connect_t connect_;
  std::mutex connect_mutex_;
};
