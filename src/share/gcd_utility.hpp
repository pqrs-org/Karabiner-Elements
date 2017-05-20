#pragma once

#include "thread_utility.hpp"
#include <dispatch/dispatch.h>
#include <sstream>
#include <string>

namespace krbn {
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

    std::string get_label(void) {
      auto p = dispatch_queue_get_label(queue_);
      return p ? p : "";
    }

  private:
    dispatch_queue_t _Nonnull queue_;
  };

  static void dispatch_sync_in_main_queue(void (^_Nonnull block)(void)) {
    if (thread_utility::is_main_thread()) {
      block();
    } else {
      dispatch_sync(dispatch_get_main_queue(), block);
    }
  }

  class main_queue_timer final {
  public:
    main_queue_timer(dispatch_time_t start, uint64_t interval, uint64_t leeway, void (^_Nonnull block)(void)) {
      timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
      if (timer_) {
        dispatch_source_set_timer(timer_, start, interval, leeway);
        dispatch_source_set_event_handler(timer_, block);
        dispatch_resume(timer_);
      }
    }

    ~main_queue_timer(void) {
      // Release timer_ in main thread to avoid callback invocations after object has been destroyed.
      dispatch_sync_in_main_queue(^{
        if (timer_) {
          dispatch_source_cancel(timer_);
          dispatch_release(timer_);
        }
      });
    }

  private:
    dispatch_source_t _Nonnull timer_;
  };

  // We want to cancel the block execution if the instance has been deleted.
  // Thus, we don't use `dispatch_after`.
  class main_queue_after_timer final {
  public:
    main_queue_after_timer(dispatch_time_t when, void (^_Nonnull block)(void)) {
      timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
      if (timer_) {
        uint64_t interval = 100.0 * NSEC_PER_SEC; /* dummy value */
        dispatch_source_set_timer(timer_, when, interval, 0);
        dispatch_source_set_event_handler(timer_, ^{
          cancel();
          block();
        });
        dispatch_resume(timer_);
      }
    }

    ~main_queue_after_timer(void) {
      cancel();
    }

    bool fired(void) const {
      bool __block r;
      gcd_utility::dispatch_sync_in_main_queue(^{
        r = !timer_;
      });
      return r;
    }

  private:
    void cancel(void) {
      // Release timer_ in main thread to avoid callback invocations after object has been destroyed.
      gcd_utility::dispatch_sync_in_main_queue(^{
        if (timer_) {
          dispatch_source_cancel(timer_);
          dispatch_release(timer_);
          timer_ = nullptr;
        }
      });
    }

    dispatch_source_t _Nonnull timer_;
  };

private:
  static std::string get_next_queue_label(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static int id = 0;

    std::stringstream stream;
    stream << "org.pqrs.gcd_utility." << id++;
    return stream.str();
  }
};
} // namespace krbn
