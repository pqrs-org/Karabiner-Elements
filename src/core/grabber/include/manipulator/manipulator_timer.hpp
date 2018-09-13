#pragma once

// `krbn::manipulator_timer` can be used safely in a multi-threaded environment.

#include "manipulator_object_id.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include <deque>

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

      if (!manipulator_object_ids_.empty()) {
        logger::get_logger().error("manipulator_timer::manipulator_object_ids_ is not empty in ~manipulator_timer.");
      }
    });

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void async_attach(manipulator_object_id id) {
    dispatcher_->enqueue([this, id] {
      manipulator_object_ids_.insert(id);
    });
  }

  void detach(manipulator_object_id id) {
    thread_utility::wait wait;

    dispatcher_->enqueue([this, id, &wait] {
      manipulator_object_ids_.erase(id);

      wait.notify();
    });

    wait.wait_notice();
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

  void async_erase(manipulator_object_id id) {
    dispatcher_->enqueue([this, id] {
      entries_.erase(std::remove_if(std::begin(entries_),
                                    std::end(entries_),
                                    [&](auto&& e) {
                                      return e->get_manipulator_object_id() == id;
                                    }),
                     std::end(entries_));
    });
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

      dispatcher_->enqueue([this, e] {
        auto it = manipulator_object_ids_.find(e->get_manipulator_object_id());
        if (it == std::end(manipulator_object_ids_)) {
          return;
        }

        e->call_function();
      });

      entries_.pop_front();
    }

    set_timer(now);
  }

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  std::unordered_set<manipulator_object_id> manipulator_object_ids_;
  std::deque<std::shared_ptr<entry>> entries_;
  bool timer_enabled_;
  std::unique_ptr<thread_utility::timer> timer_;
  boost::optional<absolute_time> timer_when_;
};
} // namespace manipulator
} // namespace krbn
