#pragma once

// `krbn::console_user_id_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "session.hpp"
#include "thread_utility.hpp"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>

namespace krbn {
class console_user_id_monitor final {
public:
  // Signals

  boost::signals2::signal<void(boost::optional<uid_t>)> console_user_id_changed;

  // Methods

  console_user_id_monitor(const console_user_id_monitor&) = delete;

  console_user_id_monitor(void) {
    queue_ = std::make_unique<thread_utility::queue>();
  }

  ~console_user_id_monitor(void) {
    async_stop();

    queue_->terminate();
    queue_ = nullptr;
  }

  void async_start(void) {
    queue_->push_back([this] {
      timer_ = std::make_unique<thread_utility::timer>(
          [](auto&& count) {
            if (count == 0) {
              return std::chrono::milliseconds(0);
            } else {
              return std::chrono::milliseconds(1000);
            }
          },
          true,
          [this] {
            queue_->push_back([this] {
              auto u = session::get_current_console_user_id();
              if (uid_ && *uid_ == u) {
                return;
              }

              console_user_id_changed(u);
              uid_ = std::make_unique<boost::optional<uid_t>>(u);
            });
          });
    });
  }

  void async_stop(void) {
    queue_->push_back([this] {
      timer_ = nullptr;
    });
  }

private:
  std::unique_ptr<thread_utility::queue> queue_;
  std::unique_ptr<thread_utility::timer> timer_;
  std::unique_ptr<boost::optional<uid_t>> uid_;
};
} // namespace krbn
