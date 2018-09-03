#pragma once

// `krbn::manipulator_timer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include <boost/signals2.hpp>
#include <deque>
#include <mach/mach_time.h>

namespace krbn {
namespace manipulator {
class manipulator_timer final {
public:
  struct timer_id : type_safe::strong_typedef<timer_id, uint64_t>,
                    type_safe::strong_typedef_op::equality_comparison<timer_id>,
                    type_safe::strong_typedef_op::relational_comparison<timer_id>,
                    type_safe::strong_typedef_op::integer_arithmetic<timer_id> {
    using strong_typedef::strong_typedef;
  };

  // Signals

  boost::signals2::signal<void(timer_id, absolute_time)> timer_invoked;

  // Methods

  class entry final {
  public:
    entry(absolute_time when) : when_(when) {
      static std::mutex mutex;
      std::lock_guard<std::mutex> guard(mutex);

      static timer_id id(0);
      timer_id_ = ++id;
    }

    timer_id get_timer_id(void) const {
      return timer_id_;
    }

    absolute_time get_when(void) const {
      return when_;
    }

    bool compare(const entry& other) {
      if (when_ != other.when_) {
        return when_ < other.when_;
      } else {
        return timer_id_ < other.timer_id_;
      }
    }

  private:
    timer_id timer_id_;
    absolute_time when_;
  };

  manipulator_timer(void) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();
  }

  ~manipulator_timer(void) {
    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void enqueue(const entry& entry,
               absolute_time now) {
    dispatcher_->enqueue([this, now, entry] {
      entries_.push_back(entry);

      std::sort(std::begin(entries_),
                std::end(entries_),
                [](auto& a, auto& b) {
                  return a.compare(b);
                });

      set_timer(now);
    });
  }

  void async_signal(absolute_time now) {
    dispatcher_->enqueue([this, now] {
      signal(now);
    });
  }

private:
  void set_timer(absolute_time now) {
    timer_ = nullptr;

    if (entries_.empty()) {
      return;
    }

    auto e = entries_.front();

    if (now < e.get_when()) {
      timer_ = std::make_unique<thread_utility::timer>(
          time_utility::absolute_to_milliseconds(e.get_when() - now),
          false,
          [this, e] {
            dispatcher_->enqueue([this, e] {
              signal(e.get_when());
            });
          });
    } else {
      signal(now);
    }
  }

  void signal(absolute_time now) {
    while (true) {
      if (entries_.empty()) {
        break;
      }

      auto e = entries_.front();

      if (now < e.get_when()) {
        break;
      }

      dispatcher_->enqueue([this, now, e] {
        timer_invoked(e.get_timer_id(), now);
      });

      entries_.pop_front();
    }

    set_timer(now);
  }

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  std::deque<entry> entries_;
  std::unique_ptr<thread_utility::timer> timer_;
};
} // namespace manipulator
} // namespace krbn
