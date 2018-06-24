#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "session.hpp"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>

namespace krbn {
class console_user_id_monitor final {
public:
  boost::signals2::signal<void(boost::optional<uid_t>)> console_user_id_changed;

  console_user_id_monitor(const console_user_id_monitor&) = delete;

  console_user_id_monitor(void) {
    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
        1 * NSEC_PER_SEC,
        0,
        ^{
          auto u = session::get_current_console_user_id();
          if (uid_ && *uid_ == u) {
            return;
          }

          console_user_id_changed(u);
          uid_ = std::make_unique<boost::optional<uid_t>>(u);
        });
  }

  ~console_user_id_monitor(void) {
    timer_ = nullptr;
  }

private:
  std::unique_ptr<gcd_utility::main_queue_timer> timer_;
  std::unique_ptr<boost::optional<uid_t>> uid_;
};
} // namespace krbn
