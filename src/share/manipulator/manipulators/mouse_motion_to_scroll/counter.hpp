#pragma once

#include "counter_chunk_value.hpp"
#include "counter_direction.hpp"
#include "counter_entry.hpp"
#include "counter_parameters.hpp"
#include "types/absolute_time_duration.hpp"
#include "types/pointing_motion.hpp"
#include <algorithm>
#include <deque>
#include <nod/nod.hpp>
#include <numeric>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/chrono.hpp>
#include <pqrs/sign.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace mouse_motion_to_scroll {
class counter final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const pointing_motion&)> scroll_event_arrived;

  // Methods

  counter(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
          const counter_parameters& parameters) : dispatcher_client(weak_dispatcher),
                                                  parameters_(parameters),
                                                  counter_direction_(counter_direction::none),
                                                  total_abs_x_(0),
                                                  total_abs_y_(0),
                                                  total_x_(0),
                                                  total_y_(0),
                                                  momentum_x_(0),
                                                  momentum_y_(0),
                                                  momentum_count_(0),
                                                  momentum_timer_(*this) {
  }

  ~counter(void) {
    detach_from_dispatcher([this] {
      momentum_timer_.stop();
    });
  }

  void update(const pointing_motion& motion, absolute_time_point time_stamp) {
    enqueue_to_dispatcher([this, motion, time_stamp] {
      entries_.emplace_back(motion.get_x(),
                            motion.get_y(),
                            time_stamp);
    });

    enqueue_to_dispatcher(
        [this, time_stamp] {
          if (entries_.empty()) {
            return;
          }
          if (time_stamp < entries_.front().get_time_stamp()) {
            return;
          }

          bool initial = false;

          {
            auto duration_milliseconds = pqrs::osx::chrono::make_milliseconds(time_stamp - *last_time_stamp_);
            if (!last_time_stamp_ ||
                duration_milliseconds > parameters_.get_recent_time_duration_milliseconds()) {
              initial = true;
              reset();
            }
          }

          // Accumulate chunk_x,chunk_y

          counter_chunk_value chunk_x;
          counter_chunk_value chunk_y;

          while (!entries_.empty()) {
            auto t = entries_.front().get_time_stamp();
            auto duration_milliseconds = pqrs::osx::chrono::make_milliseconds(t - time_stamp);
            if (duration_milliseconds < parameters_.get_recent_time_duration_milliseconds()) {
              auto x = entries_.front().get_x();
              auto y = entries_.front().get_y();

              chunk_x.add(x);
              chunk_y.add(y);

              last_time_stamp_ = t;
              entries_.pop_front();
            } else {
              break;
            }
          }

          auto x = chunk_x.make_accumulated_value();
          auto y = chunk_y.make_accumulated_value();

          // Apply direction

          if (counter_direction_ == counter_direction::horizontal) {
            y = 0;
          } else if (counter_direction_ == counter_direction::vertical) {
            x = 0;
          }

          // Reset if sign is changed.

          if (x != 0 && pqrs::make_sign(total_x_) != pqrs::make_sign(x)) {
            // Keep total_abs_x_
            total_x_ = 0;
            momentum_x_ = 0;
            initial = true;
          }
          if (y != 0 && pqrs::make_sign(total_y_) != pqrs::make_sign(y)) {
            // Keep total_abs_y_
            total_y_ = 0;
            momentum_y_ = 0;
            initial = true;
          }

          // Modify total_*

          total_abs_x_ += std::abs(x);
          total_abs_y_ += std::abs(y);
          total_x_ += x;
          total_y_ += y;

          if (total_x_ == 0 && total_y_ == 0) {
            return;
          }

          // Lock direction

          if (counter_direction_ == counter_direction::none) {
            if (total_abs_x_ > total_abs_y_) {
              counter_direction_ = counter_direction::horizontal;
              total_y_ = 0;
            } else {
              counter_direction_ = counter_direction::vertical;
              total_x_ = 0;
            }
          }

          // Enlarge total_x, total_y if initial event

          if (initial) {
            if (0 < total_x_ && total_x_ < parameters_.get_threshold()) {
              total_x_ = parameters_.get_threshold();
            } else if (-parameters_.get_threshold() < total_x_ && total_x_ < 0) {
              total_x_ = -parameters_.get_threshold();
            }

            if (0 < total_y_ && total_y_ < parameters_.get_threshold()) {
              total_y_ = parameters_.get_threshold();
            } else if (-parameters_.get_threshold() < total_y_ && total_y_ < 0) {
              total_y_ = -parameters_.get_threshold();
            }
          }

          // Update momentum values

          momentum_count_ = 0;

          momentum_timer_.start(
              [this] {
                if (!momentum_scroll()) {
                  momentum_timer_.stop();
                }
              },
              std::chrono::milliseconds(20));
        },
        when_now() + parameters_.get_recent_time_duration_milliseconds() * 2);
  }

  void async_reset(void) {
    enqueue_to_dispatcher([this] {
      reset();
    });
  }

private:
  void reset(void) {
    entries_.empty();
    last_time_stamp_ = std::nullopt;
    counter_direction_ = counter_direction::none;
    total_abs_x_ = 0;
    total_abs_y_ = 0;
    total_x_ = 0;
    total_y_ = 0;
    momentum_x_ = 0;
    momentum_y_ = 0;
    momentum_count_ = 0;
  }

  bool momentum_scroll(void) {
    ++momentum_count_;
    if (momentum_count_ == 0) {
      return false;
    }

    int dx = total_x_ * (1.0 / momentum_count_) * parameters_.get_speed_multiplier();
    int dy = total_y_ * (1.0 / momentum_count_) * parameters_.get_speed_multiplier();

    if (dx == 0 && dy == 0) {
      return false;
    }

    momentum_x_ += dx;
    momentum_y_ += dy;

#if 0
    std::cout << "tx,ty: " << total_x_ << "," << total_y_ << std::endl;
    std::cout << "dx,dy: " << dx << "," << dy << std::endl;
    std::cout << "mx,my: " << momentum_x_ << "," << momentum_y_ << std::endl;
#endif

    int x = momentum_x_ / parameters_.get_threshold();
    int y = momentum_y_ / parameters_.get_threshold();
    if (x != 0 || y != 0) {
      if (x != 0) {
        momentum_x_ -= x * parameters_.get_threshold();
      }
      if (y != 0) {
        momentum_y_ -= y * parameters_.get_threshold();
      }

      pointing_motion motion(0,
                             0,
                             -y,
                             x);
      enqueue_to_dispatcher([this, motion] {
        scroll_event_arrived(motion);
      });
    }

    reduce(total_x_, parameters_.get_momentum_minus());
    reduce(total_y_, parameters_.get_momentum_minus());

    return true;
  }

  void reduce(int& value, int amount) const {
    if (value > 0) {
      value -= std::min(amount, value);
    } else if (value < 0) {
      value += std::min(amount, -value);
    }
  }

  const counter_parameters parameters_;

  std::deque<counter_entry> entries_;
  std::optional<absolute_time_point> last_time_stamp_;

  counter_direction counter_direction_;

  // For direction detection.
  int total_abs_x_;
  int total_abs_y_;

  int total_x_;
  int total_y_;

  int momentum_x_;
  int momentum_y_;
  int momentum_count_;

  pqrs::dispatcher::extra::timer momentum_timer_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
