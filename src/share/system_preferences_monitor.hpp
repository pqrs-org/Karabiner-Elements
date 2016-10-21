#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "system_preferences.hpp"
#include <boost/optional.hpp>
#include <spdlog/spdlog.h>

class system_preferences_monitor final {
public:
  typedef std::function<void(const system_preferences::values& values)> values_updated_callback;

  system_preferences_monitor(spdlog::logger& logger,
                             const values_updated_callback& callback) : logger_(logger),
                                                                        callback_(callback) {
    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        0,
        dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
        1.0 * NSEC_PER_SEC,
        0,
        ^{
          system_preferences::values v;
          if (!values_ || *values_ != v) {
            logger_.info("system_preferences::values is updated.");

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
  spdlog::logger& logger_;
  values_updated_callback callback_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  boost::optional<system_preferences::values> values_;
};
