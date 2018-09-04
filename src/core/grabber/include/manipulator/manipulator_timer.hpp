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
  class entry final {
  public:
    entry(const std::function<void(void)>& function,
          absolute_time when) : function_(function),
                                when_(when) {
    }

    absolute_time get_when(void) const {
      return when_;
    }

    void call_function(void) const {
      function_();
    }

  private:
    std::function<void(void)> function_;
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

      std::stable_sort(std::begin(entries_),
                       std::end(entries_),
                       [](auto& a, auto& b) {
                         return a.get_when() < b.get_when();
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

      dispatcher_->enqueue([e] {
        e.call_function();
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
