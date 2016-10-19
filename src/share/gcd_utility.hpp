#pragma once

#include <sstream>
#include <string>

class gcd_utility final {
public:
  class scoped_queue final {
  public:
    scoped_queue(void) {
      auto label = get_next_queue_label();
      queue_ = dispatch_queue_create(label.c_str(), nullptr);
    }

    ~scoped_queue(void) {
      dispatch_release(queue_);
    }

    dispatch_queue_t _Nonnull get(void) { return queue_; }

  private:
    dispatch_queue_t _Nonnull queue_;
  };

private:
  static std::string get_next_queue_label(void) {
    static std::mutex mutex;
    static int id = 0;

    std::lock_guard<std::mutex> guard(mutex);

    std::stringstream stream;
    stream << "org.pqrs.gcd_utility." << id++;
    return stream.str();
  }
};
