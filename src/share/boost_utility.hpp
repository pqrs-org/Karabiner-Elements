#pragma once

// `krbn::boost_utility::signals2_connections` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include <boost/signals2.hpp>

namespace krbn {
class boost_utility final {
public:
  struct signals2_combiner_call_while_true final {
    typedef bool result_type;

    template <typename input_iterator>
    result_type operator()(input_iterator first_observer,
                           input_iterator last_observer) const {
      result_type value = true;
      for (;
           first_observer != last_observer && value;
           std::advance(first_observer, 1)) {
        value = *first_observer;
      }
      return value;
    }
  };

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
