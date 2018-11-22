#pragma once

// `krbn::boost_utility::signals2_connections` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include <boost/signals2.hpp>
#include <thread>

namespace krbn {
class boost_utility final {
public:
  class signals2_connections final {
  public:
    void push_back(const boost::signals2::connection& connection) {
      std::lock_guard<std::mutex> lock(connections_mutex_);

      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(connection));
    }

    void disconnect_all_connections(void) {
      {
        std::lock_guard<std::mutex> lock(connections_mutex_);

        connections_.clear();
      }

      connections_cv_.notify_one();
    }

    void wait_disconnect_all_connections(void) {
      std::unique_lock<std::mutex> lock(connections_mutex_);

      connections_cv_.wait(lock, [this] {
        return connections_.empty();
      });
    }

  private:
    std::vector<std::unique_ptr<boost::signals2::scoped_connection>> connections_;
    mutable std::mutex connections_mutex_;
    std::condition_variable connections_cv_;
  };
};
} // namespace krbn
