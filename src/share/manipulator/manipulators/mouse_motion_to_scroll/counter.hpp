#pragma once

#include "core_configuration/core_configuration.hpp"
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
          const core_configuration::details::complex_modifications_parameters& parameters,
          const counter_parameters& counter_parameters) : dispatcher_client(weak_dispatcher),
                                                          parameters_(parameters),
                                                          counter_parameters_(counter_parameters),
                                                          counter_direction_(counter_direction::none),
                                                          total_x_(0),
                                                          total_y_(0),
                                                          momentum_x_(0),
                                                          momentum_y_(0),
                                                          momentum_count_(0),
                                                          momentum_wait_(0),
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
                duration_milliseconds > counter_parameters_.get_recent_time_duration_milliseconds()) {
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
            if (duration_milliseconds < counter_parameters_.get_recent_time_duration_milliseconds()) {
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

          total_x_ += x;
          total_y_ += y;

          if (total_x_ == 0 && total_y_ == 0) {
            return;
          }

          // Lock direction

          if (counter_direction_ == counter_direction::none) {
            if (chunk_x.get_abs_total() > chunk_y.get_abs_total()) {
              counter_direction_ = counter_direction::horizontal;
              total_y_ = 0;
            } else {
              counter_direction_ = counter_direction::vertical;
              total_x_ = 0;
            }
          }

          // Enlarge total_x, total_y if initial event

          if (initial) {
            int least_value = round_up(counter_parameters_.get_threshold() /
                                       parameters_.make_mouse_motion_to_scroll_speed_rate() /
                                       counter_parameters_.get_speed_multiplier());
#if 0
            std::cout << "least_value " << least_value << std::endl;
#endif
            if (least_value == 0) {
              least_value = 1;
            }

            if (0 < total_x_) {
              total_x_ = std::max(total_x_, least_value);
            } else if (total_x_ < 0) {
              total_x_ = std::min(total_x_, -least_value);
            }

            if (0 < total_y_) {
              total_y_ = std::max(total_y_, least_value);
            } else if (total_y_ < 0) {
              total_y_ = std::min(total_y_, -least_value);
            }
          }

          // Update momentum values

          momentum_count_ = 0;
          momentum_wait_ = 0;

          momentum_timer_.start(
              [this] {
                if (!momentum_scroll()) {
                  momentum_timer_.stop();
                }
              },
              std::chrono::milliseconds(20));
        },
        when_now() + counter_parameters_.get_recent_time_duration_milliseconds() * 2);
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
    total_x_ = 0;
    total_y_ = 0;
    momentum_x_ = 0;
    momentum_y_ = 0;
    momentum_count_ = 0;
  }

  bool momentum_scroll(void) {
    if (momentum_wait_ > 0) {
      --momentum_wait_;
      return true;
    }

    ++momentum_count_;
    if (momentum_count_ == 0) {
      return false;
    }

    double multiplier = parameters_.make_mouse_motion_to_scroll_speed_rate() *
                        counter_parameters_.get_speed_multiplier();
    if (multiplier == 0.0) {
      return false;
    }

    double scale = (1.0 / momentum_count_) * multiplier;
    if (scale == 0.0) {
      return false;
    }

    int dx = round_up(total_x_ * scale);
    int dy = round_up(total_y_ * scale);

    momentum_x_ += dx;
    momentum_y_ += dy;

    auto x = convert(momentum_x_);
    auto y = convert(momentum_y_);

#if 0
    std::cout << "multiplier: " << multiplier << std::endl;
    std::cout << "scape: " << scale << std::endl;
    std::cout << "tx,ty: " << total_x_ << "," << total_y_ << std::endl;
    std::cout << "dx,dy: " << dx << "," << dy << std::endl;
    std::cout << "mx,my: " << momentum_x_ << "," << momentum_y_ << std::endl;
    std::cout << "x,y: " << x << "," << y << std::endl;
#endif

    if (x != 0 || y != 0) {
      pointing_motion motion(0,
                             0,
                             -y,
                             x);
      enqueue_to_dispatcher([this, motion] {
        scroll_event_arrived(motion);
      });
    }

    reduce(total_x_, counter_parameters_.get_momentum_minus() / multiplier);
    reduce(total_y_, counter_parameters_.get_momentum_minus() / multiplier);

    if (total_x_ == 0 && total_y_ == 0) {
      return false;
    }

    momentum_wait_ = momentum_count_;

    return true;
  }

  int round_up(double value) const {
    if (value > 0) {
      return std::ceil(value);
    } else if (value < 0) {
      return std::ceil(value) - 1;
    }
    return 0;
  }

  int convert(int& value) const {
    int threshold = counter_parameters_.get_threshold();
    if (value >= threshold) {
      value -= threshold;
      return 1;
    }
    if (value <= -threshold) {
      value += threshold;
      return -1;
    }
    return 0;
  }

  void reduce(int& value, int amount) const {
    if (value > 0) {
      value -= std::min(amount, value);
    } else if (value < 0) {
      value += std::min(amount, -value);
    }
  }

  const core_configuration::details::complex_modifications_parameters parameters_;
  const counter_parameters counter_parameters_;

  std::deque<counter_entry> entries_;
  std::optional<absolute_time_point> last_time_stamp_;

  counter_direction counter_direction_;

  int total_x_;
  int total_y_;

  int momentum_x_;
  int momentum_y_;
  int momentum_count_;
  int momentum_wait_;

  pqrs::dispatcher::extra::timer momentum_timer_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
