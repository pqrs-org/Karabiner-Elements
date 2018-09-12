#pragma once

// `krbn::manipulator_timer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "manipulator_object_id.hpp"
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
  manipulator_timer(bool timer_enabled = true) : timer_enabled_(timer_enabled) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();
  }

  ~manipulator_timer(void) {
    dispatcher_->enqueue([this] {
      // Stop `timer_` gracefully without timer_->cancel().
      timer_ = nullptr;
      timer_when_ = boost::none;
    });

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void enqueue(manipulator_object_id id,
               const std::function<void(void)>& function,
               absolute_time when) {
    dispatcher_->enqueue([this, id, function, when] {
      entries_.push_back(std::make_shared<entry>(id, function, when));

      std::stable_sort(std::begin(entries_),
                       std::end(entries_),
                       [](auto& a, auto& b) {
                         return a->get_when() < b->get_when();
                       });
    });
  }

  void async_erase(manipulator_object_id id,
                   const std::function<void(void)>& erased) {
    dispatcher_->enqueue([this, id, erased] {
      entries_.erase(std::remove_if(std::begin(entries_),
                                    std::end(entries_),
                                    [&](auto&& e) {
                                      return e->get_manipulator_object_id() == id;
                                    }),
                     std::end(entries_));

      erased();
    });
  }

  void async_erase(manipulator_object_id id) {
    async_erase(id, [] {});
  }

  void async_invoke(absolute_time now) {
    dispatcher_->enqueue([this, now] {
      invoke(now);
    });
  }

private:
  class entry final {
  public:
    entry(manipulator_object_id id,
          const std::function<void(void)>& function,
          absolute_time when) : manipulator_object_id_(id),
                                function_(function),
                                when_(when) {
    }

    manipulator_object_id get_manipulator_object_id(void) const {
      return manipulator_object_id_;
    }

    absolute_time get_when(void) const {
      return when_;
    }

    void call_function(void) const {
      function_();
    }

  private:
    manipulator_object_id manipulator_object_id_;
    std::function<void(void)> function_;
    absolute_time when_;
  };

  void set_timer(absolute_time now) {
    if (entries_.empty()) {
      if (timer_) {
        timer_->cancel();
        timer_ = nullptr;
      }
      return;
    }

    auto e = entries_.front();

    if (now < e->get_when()) {
      if (timer_enabled_) {
        // Skip if timer_ is active.

        if (timer_ &&
            timer_when_ &&
            *timer_when_ == e->get_when()) {
          return;
        }

        // Update timer_.

        if (timer_) {
          timer_->cancel();
          timer_ = nullptr;
        }

        timer_when_ = e->get_when();
        timer_ = std::make_unique<thread_utility::timer>(
            time_utility::to_milliseconds(e->get_when() - now),
            thread_utility::timer::mode::once,
            [this, e] {
              dispatcher_->enqueue([this, e] {
                timer_when_ = boost::none;
                invoke(e->get_when());
              });
            });
      }
    } else {
      if (timer_) {
        timer_->cancel();
        timer_ = nullptr;
      }

      invoke(now);
    }
  }

  void invoke(absolute_time now) {
    while (true) {
      if (entries_.empty()) {
        break;
      }

      auto e = entries_.front();

      if (now < e->get_when()) {
        break;
      }

      dispatcher_->enqueue([e] {
        e->call_function();
      });

      entries_.pop_front();
    }

    set_timer(now);
  }

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  std::deque<std::shared_ptr<entry>> entries_;
  bool timer_enabled_;
  std::unique_ptr<thread_utility::timer> timer_;
  boost::optional<absolute_time> timer_when_;
};
} // namespace manipulator
} // namespace krbn
