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
  struct client_id : type_safe::strong_typedef<client_id, std::intptr_t>,
                     type_safe::strong_typedef_op::equality_comparison<client_id>,
                     type_safe::strong_typedef_op::integer_arithmetic<client_id> {
    using strong_typedef::strong_typedef;
  };

  static client_id make_client_id(void* p) {
    return client_id(reinterpret_cast<std::intptr_t>(p));
  }

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

  void enqueue(client_id client_id,
               const std::function<void(void)>& function,
               absolute_time when) {
    dispatcher_->enqueue([this, client_id, function, when] {
      entries_.push_back(std::make_shared<entry>(client_id, function, when));

      std::stable_sort(std::begin(entries_),
                       std::end(entries_),
                       [](auto& a, auto& b) {
                         return a->get_when() < b->get_when();
                       });
    });
  }

  void async_erase(client_id client_id,
                   const std::function<void(void)>& erased) {
    dispatcher_->enqueue([this, client_id, erased] {
      entries_.erase(std::remove_if(std::begin(entries_),
                                    std::end(entries_),
                                    [&](auto&& e) {
                                      return e->get_client_id() == client_id;
                                    }),
                     std::end(entries_));

      erased();
    });
  }

  void async_erase(client_id client_id) {
    async_erase(client_id, [] {});
  }

  void async_invoke(absolute_time now) {
    dispatcher_->enqueue([this, now] {
      invoke(now);
    });
  }

private:
  class entry final {
  public:
    entry(client_id client_id,
          const std::function<void(void)>& function,
          absolute_time when) : client_id_(client_id),
                                function_(function),
                                when_(when) {
    }

    client_id get_client_id(void) const {
      return client_id_;
    }

    absolute_time get_when(void) const {
      return when_;
    }

    void call_function(void) const {
      function_();
    }

  private:
    client_id client_id_;
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
