#pragma once

class user_client final {
public:
  user_client(void) : connect_(IO_OBJECT_NULL) {
  }

  ~user_client(void) {
    close();
  }

  bool open(const std::string& class_name, uint32_t type) {
    if (connect_ != IO_OBJECT_NULL) {
      return true;
    }

    io_iterator_t iterator = IO_OBJECT_NULL;
    auto kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(class_name.c_str()), &iterator);
    if (kr != KERN_SUCCESS) {
      std::cerr << "IOServiceGetMatchingServices failed" << std::endl;
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
  io_connect_t connect_;
  io_service_t service_;
};
