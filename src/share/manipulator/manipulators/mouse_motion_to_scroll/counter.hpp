#pragma once

#include "core_configuration/core_configuration.hpp"
#include "counter_chunk_value.hpp"
#include "counter_direction.hpp"
#include "counter_entry.hpp"
#include "options.hpp"
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
  using chunk_accumulated_values_entry_t = std::pair<pqrs::dispatcher::time_point, int>;
  using chunk_accumulated_values_t = std::deque<std::pair<pqrs::dispatcher::time_point, int>>;
  static constexpr int timer_interval = 20;

  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const pointing_motion&)> scroll_event_arrived;

  // Methods

  counter(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
          const core_configuration::details::complex_modifications_parameters& parameters,
          const options& options) : dispatcher_client(weak_dispatcher),
                                    parameters_(parameters),
                                    options_(options),
                                    counter_direction_(counter_direction::none),
                                    total_x_(0),
                                    total_y_(0),
                                    momentum_x_(0),
                                    momentum_y_(0),
                                    momentum_count_(0),
                                    momentum_wait_(0),
                                    timer_(*this) {
  }

  ~counter(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void update(const pointing_motion& motion, pqrs::dispatcher::time_point time_point) {
    enqueue_to_dispatcher([this, motion, time_point] {
      entries_.emplace_back(motion.get_x(),
                            motion.get_y(),
                            time_point);

      timer_.start(
          [this] {
            bool continue_timer = false;

            continue_timer |= process_entries();
            continue_timer |= scroll();

            if (!continue_timer) {
              timer_.stop();
            }
          },
          std::chrono::milliseconds(timer_interval));
    });
  }

  void async_reset(void) {
    enqueue_to_dispatcher([this] {
      entries_.empty();
      last_entry_time_point_ = std::nullopt;
      last_scroll_time_point_ = std::nullopt;
      counter_direction_ = counter_direction::none;
      chunk_accumulated_values_x_.clear();
      chunk_accumulated_values_y_.clear();
      total_x_ = 0;
      total_y_ = 0;
      momentum_x_ = 0;
      momentum_y_ = 0;
      momentum_count_ = 0;
      momentum_wait_ = 0;
    });
  }

private:
  bool process_entries(void) {
    if (entries_.empty()) {
      return false;
    }

    auto recent_time_duration_milliseconds = options_.get_recent_time_duration_milliseconds();
    auto front_time_point = entries_.front().get_time_point();

    if (when_now() - front_time_point <= recent_time_duration_milliseconds) {
      return true;
    }

    erase_chunk_accumulated_values(chunk_accumulated_values_x_,
                                   front_time_point);
    erase_chunk_accumulated_values(chunk_accumulated_values_y_,
                                   front_time_point);

    bool initial = false;

    if (chunk_accumulated_values_x_.empty() &&
        chunk_accumulated_values_y_.empty()) {
      initial = true;
      counter_direction_ = counter_direction::none;
    }

    // Accumulate chunk_x,chunk_y

    counter_chunk_value chunk_x;
    counter_chunk_value chunk_y;

    while (!entries_.empty()) {
      auto t = entries_.front().get_time_point();
      auto duration = t - front_time_point;
      if (duration <= recent_time_duration_milliseconds) {
        auto x = entries_.front().get_x();
        auto y = entries_.front().get_y();

        chunk_x.add(x);
        chunk_y.add(y);

        last_entry_time_point_ = t;
        entries_.pop_front();
      } else {
        break;
      }
    }

    auto x = chunk_x.make_accumulated_value();
    auto y = chunk_y.make_accumulated_value();

    // Update chunk_accumulated_values_*

    chunk_accumulated_values_x_.push_back(std::make_pair(front_time_point, x));
    chunk_accumulated_values_y_.push_back(std::make_pair(front_time_point, y));

    // Reset direction

    {
      auto recent_chunks_total_x = accumulate_abs_chunk_accumulated_values(chunk_accumulated_values_x_);
      auto recent_chunks_total_y = accumulate_abs_chunk_accumulated_values(chunk_accumulated_values_y_);

      if (counter_direction_ == counter_direction::horizontal) {
        if (recent_chunks_total_y > recent_chunks_total_x) {
          counter_direction_ = counter_direction::none;
        }
      } else if (counter_direction_ == counter_direction::vertical) {
        if (recent_chunks_total_x > recent_chunks_total_y) {
          counter_direction_ = counter_direction::none;
        }
      }
    }

    // Set direction

    if (counter_direction_ == counter_direction::none) {
      if (chunk_x.get_abs_total() > chunk_y.get_abs_total()) {
        counter_direction_ = counter_direction::horizontal;
      } else {
        counter_direction_ = counter_direction::vertical;
      }

      total_x_ = 0;
      total_y_ = 0;
      momentum_x_ = 0;
      momentum_y_ = 0;
      momentum_minus_ = options_.get_threshold();
    }

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
      momentum_minus_ = options_.get_threshold();
      initial = true;
    }
    if (y != 0 && pqrs::make_sign(total_y_) != pqrs::make_sign(y)) {
      // Keep total_abs_y_
      total_y_ = 0;
      momentum_y_ = 0;
      momentum_minus_ = options_.get_threshold();
      initial = true;
    }

    // Multiply x,y

    double multiplier = parameters_.make_mouse_motion_to_scroll_speed_rate() *
                        options_.get_speed_multiplier();
    x *= multiplier;
    y *= multiplier;

    // Modify total_*

    total_x_ += x;
    total_y_ += y;

    if (total_x_ == 0 && total_y_ == 0) {
      return true;
    }

    // Enlarge total_x, total_y if initial event

    if (initial) {
      int least_value = options_.get_threshold();
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

    {
      auto value = static_cast<double>(std::max(std::abs(total_x_), std::abs(total_y_)));
      value /= options_.get_threshold();

      if (value > 10) {
        value = 10;
      }

      auto minus = static_cast<int>(static_cast<double>(options_.get_threshold()) / std::pow(value, 4));

      if (minus <= 0) {
        minus = 1;
      }

      momentum_minus_ = std::min(momentum_minus_, minus);

#if 0
      std::cout << "total_x_ " << total_x_ << std::endl;
      std::cout << "total_y_ " << total_y_ << std::endl;
      std::cout << "value " << value << std::endl;
      std::cout << "pow " << std::pow(value, 4) << std::endl;
      std::cout << "minus " << minus << std::endl;
      std::cout << "momentum_minus_ " << momentum_minus_ << std::endl;
#endif
    }

    last_scroll_time_point_ = std::nullopt;
    momentum_count_ = 0;
    momentum_wait_ = 0;

    return true;
  }

  bool scroll(void) {
    if (momentum_wait_ > 0) {
      --momentum_wait_;
      return true;
    }

    ++momentum_count_;
    if (momentum_count_ == 0) {
      return false;
    }

    auto now = when_now();

    if (last_scroll_time_point_ &&
        now - *last_scroll_time_point_ > options_.get_scroll_event_interval_milliseconds_threshold()) {
      return false;
    }

    double scale = (1.0 / momentum_count_);
    if (!options_.get_momentum_scroll_enabled() &&
        momentum_count_ > 1) {
      scale = 0;
    }

    int dx = round_up(total_x_ * scale);
    int dy = round_up(total_y_ * scale);

    momentum_x_ += dx;
    momentum_y_ += dy;

    auto x = convert(momentum_x_);
    auto y = convert(momentum_y_);

#if 0
    std::cout << "scale: " << scale << std::endl;
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

      last_scroll_time_point_ = now;
    }

    reduce(total_x_, momentum_minus_);
    reduce(total_y_, momentum_minus_);

    if (total_x_ == 0 && total_y_ == 0) {
      return false;
    }

    if (options_.get_momentum_scroll_enabled()) {
      momentum_wait_ = std::min(momentum_count_, 10);
    }

    return true;
  }

  void erase_chunk_accumulated_values(chunk_accumulated_values_t& chunk_accumulated_values,
                                      pqrs::dispatcher::time_point time_point) const {
    auto threshold = options_.get_recent_time_duration_milliseconds() *
                     options_.get_direction_lock_threshold();

    chunk_accumulated_values.erase(std::remove_if(std::begin(chunk_accumulated_values),
                                                  std::end(chunk_accumulated_values),
                                                  [time_point, threshold](auto&& p) {
                                                    return (time_point - p.first) > threshold;
                                                  }),
                                   std::end(chunk_accumulated_values));
  }

  int accumulate_abs_chunk_accumulated_values(const chunk_accumulated_values_t& chunk_accumulated_values) const {
    return std::accumulate(std::begin(chunk_accumulated_values),
                           std::end(chunk_accumulated_values),
                           0,
                           [](auto&& a, auto&& b) {
                             return a + std::abs(b.second);
                           });
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
    int result = 0;
    int threshold = options_.get_threshold();

    while (value >= threshold) {
      value -= threshold;
      ++result;
      if (options_.get_momentum_scroll_enabled()) {
        break;
      }
    }

    while (value <= -threshold) {
      value += threshold;
      --result;
      if (options_.get_momentum_scroll_enabled()) {
        break;
      }
    }

    return result;
  }

  void reduce(int& value, int amount) const {
    if (value > 0) {
      value -= std::min(amount, value);
    } else if (value < 0) {
      value += std::min(amount, -value);
    }
  }

  const core_configuration::details::complex_modifications_parameters parameters_;
  const options options_;

  std::deque<counter_entry> entries_;
  std::optional<pqrs::dispatcher::time_point> last_entry_time_point_;
  std::optional<pqrs::dispatcher::time_point> last_scroll_time_point_;

  counter_direction counter_direction_;
  chunk_accumulated_values_t chunk_accumulated_values_x_;
  chunk_accumulated_values_t chunk_accumulated_values_y_;

  int total_x_;
  int total_y_;

  int momentum_x_;
  int momentum_y_;
  int momentum_minus_;
  int momentum_count_;
  int momentum_wait_;

  pqrs::dispatcher::extra::timer timer_;
};
} // namespace mouse_motion_to_scroll
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
