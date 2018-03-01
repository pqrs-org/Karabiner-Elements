#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include <boost/signals2.hpp>
#include <deque>
#include <mach/mach_time.h>

namespace krbn {
namespace manipulator {
class manipulator_timer final {
public:
  enum class timer_id : uint64_t {
    zero = 0,
  };

  class entry final {
  public:
    entry(uint64_t when) : when_(when) {
      static std::mutex mutex;
      std::lock_guard<std::mutex> guard(mutex);

      static timer_id id;
      id = timer_id(static_cast<uint64_t>(id) + 1);
      timer_id_ = id;
    }

    timer_id get_timer_id(void) const {
      return timer_id_;
    }

    uint64_t get_when(void) const {
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
    uint64_t when_;
  };

  class core final {
  public:
    boost::signals2::signal<void(timer_id, uint64_t)> timer_invoked;

    core(void) : enabled_(false) {
    }

    // For unit testing
    const std::deque<entry>& get_entries(void) const {
      return entries_;
    }

    void enable(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      enabled_ = true;
    }

    void disable(void) {
      std::lock_guard<std::mutex> guard(mutex_);

      enabled_ = false;
    }

    timer_id add_entry(uint64_t when) {
      std::lock_guard<std::mutex> guard(mutex_);

      entries_.emplace_back(when);
      auto result = entries_.back().get_timer_id();

      std::sort(std::begin(entries_),
                std::end(entries_),
                [](auto& a, auto& b) {
                  return a.compare(b);
                });

      set_timer();

      return result;
    }

    void signal(uint64_t now) {
      for (;;) {
        boost::optional<timer_id> id;

        {
          std::lock_guard<std::mutex> guard(mutex_);

          if (!entries_.empty() && entries_.front().get_when() <= now) {
            id = entries_.front().get_timer_id();
            entries_.pop_front();
          }
        }

        if (id) {
          timer_invoked(*id, now);
        } else {
          break;
        }
      }

      set_timer();
    }

  private:
    void set_timer(void) {
      if (!enabled_) {
        timer_ = nullptr;
        return;
      }

      if (entries_.empty()) {
        timer_ = nullptr;
        return;
      }

      timer_ = std::make_unique<gcd_utility::main_queue_after_timer>(entries_.front().get_when(),
                                                                     true,
                                                                     ^{
                                                                       uint64_t now = mach_absolute_time();
                                                                       signal(now);
                                                                     });
    }

    std::mutex mutex_;
    bool enabled_;
    std::deque<entry> entries_;
    std::unique_ptr<gcd_utility::main_queue_after_timer> timer_;
  };

  static core& get_instance(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unique_ptr<core> core_;
    if (!core_) {
      core_ = std::make_unique<core>();
    }

    return *core_;
  }
};
} // namespace manipulator
} // namespace krbn
