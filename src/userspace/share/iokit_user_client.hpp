#pragma once

#include <IOKit/IOKitLib.h>
#include <list>
#include <spdlog/spdlog.h>

#include "iokit_utility.hpp"

class iokit_user_client final {
public:
  iokit_user_client(spdlog::logger& logger, const std::string& class_name, uint32_t type) : logger_(logger),
                                                                                            class_name_(class_name),
                                                                                            type_(type),
                                                                                            notification_port_(nullptr),
                                                                                            matched_notification_(IO_OBJECT_NULL),
                                                                                            terminated_notification_(IO_OBJECT_NULL),
                                                                                            connect_(IO_OBJECT_NULL),
                                                                                            service_(IO_OBJECT_NULL) {
    notification_port_ = IONotificationPortCreate(kIOMasterPortDefault);
    if (notification_port_) {
      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        CFRunLoopAddSource(CFRunLoopGetCurrent(), loop_source, kCFRunLoopDefaultMode);
      }
    }
  }

  ~iokit_user_client(void) {
    close();

    if (matched_notification_) {
      IOObjectRelease(matched_notification_);
      matched_notification_ = IO_OBJECT_NULL;
    }

    if (terminated_notification_) {
      IOObjectRelease(terminated_notification_);
      terminated_notification_ = IO_OBJECT_NULL;
    }

    if (notification_port_) {
      if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), loop_source, kCFRunLoopDefaultMode);
      }

      IONotificationPortDestroy(notification_port_);
      notification_port_ = nullptr;
    }
  }

  void start() {
    logger_.info("iokit_user_client::start");

    if (!notification_port_) {
      logger_.error("notification_port_ is nullptr");
      return;
    }

    // kIOMatchedNotification
    {
      auto kr = IOServiceAddMatchingNotification(notification_port_,
                                                 kIOMatchedNotification,
                                                 IOServiceNameMatching(class_name_.c_str()),
                                                 &iokit_user_client::static_matched_callback,
                                                 static_cast<void*>(this),
                                                 &matched_notification_);
      if (kr != kIOReturnSuccess) {
        logger_.error("IOServiceAddMatchingNotification error: 0x{0:x}", kr);
      } else {
        matched_callback(matched_notification_);
      }
    }

    // kIOTerminatedNotification
    {
      auto kr = IOServiceAddMatchingNotification(notification_port_,
                                                 kIOTerminatedNotification,
                                                 IOServiceNameMatching(class_name_.c_str()),
                                                 &iokit_user_client::static_terminated_callback,
                                                 static_cast<void*>(this),
                                                 &terminated_notification_);
      if (kr != kIOReturnSuccess) {
        logger_.error("IOServiceAddMatchingNotification error: 0x{0:x}", kr);
      } else {
        terminated_callback(terminated_notification_);
      }
    }
  }

  bool open(const std::string& class_name, uint32_t type) {
    if (connect_ != IO_OBJECT_NULL) {
      return true;
    }

    io_iterator_t iterator = IO_OBJECT_NULL;
    auto kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(class_name.c_str()), &iterator);
    if (kr != KERN_SUCCESS) {
      logger_.error("IOServiceGetMatchingServices failed: class_name:{0} 0x{1:x}", class_name, kr);
      goto finish;
    }

    while (true) {
      service_ = IOIteratorNext(iterator);
      if (service_ == IO_OBJECT_NULL) {
        break;
      }

      kr = IOServiceOpen(service_, mach_task_self(), type, &connect_);
      if (kr == KERN_SUCCESS) {
        break;
      }

      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
      connect_ = IO_OBJECT_NULL;
    }

  finish:
    if (iterator) {
      IOObjectRelease(iterator);
    }

    return connect_ != IO_OBJECT_NULL;
  }

  void close() {
    if (connect_ != IO_OBJECT_NULL) {
      IOServiceClose(connect_);
      connect_ = IO_OBJECT_NULL;
    }
    if (service_ != IO_OBJECT_NULL) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }
  }

  io_connect_t get_connect(void) const { return connect_; }

private:
  class connection final {
  public:
    connection(spdlog::logger& logger, io_service_t service, uint32_t type) : logger_(logger),
                                                                              service_(service),
                                                                              connect_(IO_OBJECT_NULL) {
      if (service_) {
        IOObjectRetain(service_);

        serial_number_ = iokit_utility::get_serial_number(service_);

        auto kr = IOServiceOpen(service_, mach_task_self(), type, &connect_);
        if (kr != kIOReturnSuccess) {
          logger_.error("IOServiceOpen error: 0x{0:x}", kr);
        }
      }
    }

    ~connection(void) {
      if (connect_) {
        IOServiceClose(connect_);
        connect_ = IO_OBJECT_NULL;
      }
      if (service_) {
        IOObjectRelease(service_);
        service_ = IO_OBJECT_NULL;
      }
    }

    io_service_t get_service(void) const { return service_; }
    io_connect_t get_connect(void) const { return connect_; }
    const std::string& get_serial_number(void) const { return serial_number_; }

  private:
    spdlog::logger& logger_;
    io_service_t service_;
    io_connect_t connect_;
    std::string serial_number_;
  };

  static void static_matched_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    iokit_user_client* self = static_cast<iokit_user_client*>(refcon);
    self->matched_callback(iterator);
  }

  void matched_callback(io_iterator_t iterator) {
    while (auto service = IOIteratorNext(iterator)) {
      connections_.push_back(connection(logger_, service, type_));

      logger_.info("iokit_user_client::matched_callback: {0}", connections_.back().get_serial_number());
      logger_.info("iokit_user_client::matched_callback connections_.size():{0}", connections_.size());

      IOObjectRelease(service);
    }
  }

  static void static_terminated_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    iokit_user_client* self = static_cast<iokit_user_client*>(refcon);
    self->terminated_callback(iterator);
  }

  void terminated_callback(io_iterator_t iterator) {
    while (auto service = IOIteratorNext(iterator)) {
      auto serial_number = iokit_utility::get_serial_number(service);

      auto it = connections_.begin();
      while (it != connections_.end()) {
        if (it->get_serial_number() == serial_number) {
          it = connections_.erase(it);
        } else {
          ++it;
        }
      }

      logger_.info("iokit_user_client::terminated_callback: {0}", serial_number);
      logger_.info("iokit_user_client::terminated_callback connections_.size():{0}", connections_.size());

      IOObjectRelease(service);
    }
  }

  spdlog::logger& logger_;
  std::string class_name_;
  uint32_t type_;

  IONotificationPortRef _Nullable notification_port_;
  io_iterator_t matched_notification_;
  io_iterator_t terminated_notification_;

  std::list<connection> connections_;

  io_connect_t connect_;
  io_service_t service_;
};
