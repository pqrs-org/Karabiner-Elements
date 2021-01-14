#pragma once

// `krbn::manipulator::manipulator_managers_connector` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include "logger.hpp"
#include "manipulator/manipulator_manager.hpp"
#include <mutex>

namespace krbn {
namespace manipulator {
class manipulator_managers_connector final {
public:
  class connection final {
    friend class manipulator_managers_connector;

  public:
    connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
               std::weak_ptr<event_queue::queue> weak_input_event_queue,
               std::weak_ptr<event_queue::queue> weak_output_event_queue) : weak_manipulator_manager_(weak_manipulator_manager),
                                                                            weak_input_event_queue_(weak_input_event_queue),
                                                                            weak_output_event_queue_(weak_output_event_queue) {
    }

  private:
    std::weak_ptr<event_queue::queue> get_weak_output_event_queue(void) const {
      return weak_output_event_queue_;
    }

    void manipulate(absolute_time_point now) const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        while (true) {
          auto processed = manipulator_manager->manipulate(weak_input_event_queue_,
                                                           weak_output_event_queue_,
                                                           now);
          if (!processed) {
            break;
          }
        }
      }
    }

    void invalidate_manipulators(void) const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        manipulator_manager->invalidate_manipulators();
      }
    }

    bool needs_virtual_hid_pointing(void) const {
      if (auto manipulator_manager = weak_manipulator_manager_.lock()) {
        return manipulator_manager->needs_virtual_hid_pointing();
      }
      return false;
    }

    std::optional<absolute_time_point> make_input_event_time_stamp_with_input_delay(void) const {
      if (auto input_event_queue = weak_input_event_queue_.lock()) {
        if (!input_event_queue->get_entries().empty()) {
          return input_event_queue->get_entries().front().get_event_time_stamp().make_time_stamp_with_input_delay();
        }
      }
      return std::nullopt;
    }

    void log_events_sizes(void) const {
      if (auto input_event_queue = weak_input_event_queue_.lock()) {
        if (auto output_event_queue = weak_output_event_queue_.lock()) {
          logger::get_logger()->info("connection events sizes: {0} -> {1}",
                                     input_event_queue->get_entries().size(),
                                     output_event_queue->get_entries().size());
        }
      }
    }

    std::weak_ptr<manipulator_manager> weak_manipulator_manager_;
    std::weak_ptr<event_queue::queue> weak_input_event_queue_;
    std::weak_ptr<event_queue::queue> weak_output_event_queue_;
  };

  manipulator_managers_connector(void) {
  }

  void emplace_back_connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
                               std::weak_ptr<event_queue::queue> weak_input_event_queue,
                               std::weak_ptr<event_queue::queue> weak_output_event_queue) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    connections_.emplace_back(weak_manipulator_manager,
                              weak_input_event_queue,
                              weak_output_event_queue);
  }

  void emplace_back_connection(std::weak_ptr<manipulator_manager> weak_manipulator_manager,
                               std::weak_ptr<event_queue::queue> weak_output_event_queue) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    if (connections_.empty()) {
      throw std::runtime_error("manipulator_managers_connector::connections_ is empty");
    }

    connections_.emplace_back(weak_manipulator_manager,
                              connections_.back().get_weak_output_event_queue(),
                              weak_output_event_queue);
  }

  void manipulate(absolute_time_point now) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto&& c : connections_) {
      c.manipulate(now);
    }
  }

  void invalidate_manipulators(void) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto&& c : connections_) {
      c.invalidate_manipulators();
    }
  }

  bool needs_virtual_hid_pointing(void) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    return std::any_of(std::begin(connections_),
                       std::end(connections_),
                       [](auto& c) {
                         return c.needs_virtual_hid_pointing();
                       });
  }

  std::optional<absolute_time_point> min_input_event_time_stamp(void) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::optional<absolute_time_point> result;

    for (const auto& c : connections_) {
      if (auto t = c.make_input_event_time_stamp_with_input_delay()) {
        if (!result || *t < *result) {
          result = t;
        }
      }
    }

    return result;
  }

  void log_events_sizes(void) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (auto&& c : connections_) {
      c.log_events_sizes();
    }
  }

private:
  std::vector<connection> connections_;
  mutable std::mutex connections_mutex_;
};
} // namespace manipulator
} // namespace krbn
