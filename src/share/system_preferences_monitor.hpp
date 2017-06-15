#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
#include <boost/optional.hpp>

namespace krbn {
class system_preferences_monitor final {
public:
  typedef std::function<void(const system_preferences::values& values)> values_updated_callback;

  system_preferences_monitor(const values_updated_callback& callback) : callback_(callback) {
    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
        1.0 * NSEC_PER_SEC,
        0,
        ^{
          system_preferences::values v;
          if (!values_ || *values_ != v) {
            logger::get_logger().info("system_preferences::values is updated.");

            values_ = v;
            if (callback_) {
              callback_(*values_);
            }
          }
        });
  }

  ~system_preferences_monitor(void) {
    timer_ = nullptr;
  }

private:
  values_updated_callback callback_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  boost::optional<system_preferences::values> values_;
};
} // namespace krbn
