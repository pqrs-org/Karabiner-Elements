#pragma once

class user_client {
public:
  user_client(void) : connect_(IO_OBJECT_NULL) {
  }

  bool open(const std::string& class_name) {
    if (connect_ != IO_OBJECT_NULL) {
      return true;
    }

    io_iterator_t iterator = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(class_name.c_str()), &iterator);
    if (kr != KERN_SUCCESS) {
      std::cerr << "IOServiceGetMatchingServices failed" << std::endl;
      goto finish;
    }

    while (true) {
      service_ = IOIteratorNext(iterator);
      std::cout << "service_ " << service_ << std::endl;
      if (service_ == IO_OBJECT_NULL) {
        break;
      }

      kr = IOServiceOpen(service_, mach_task_self(), 0, &connect_);
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
  io_connect_t connect_;
  io_service_t service_;
};
