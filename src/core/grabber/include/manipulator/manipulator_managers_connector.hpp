#pragma once

#include "event_queue.hpp"
#include "logger.hpp"
#include "manipulator/manipulator_manager.hpp"

namespace krbn {
namespace manipulator {
class manipulator_managers_connector final {
public:
  class connection final {
  public:
    connection(manipulator_manager& manipulator_manager,
               const std::shared_ptr<event_queue>& input_event_queue,
               const std::shared_ptr<event_queue>& output_event_queue) : manipulator_manager_(manipulator_manager),
                                                                         input_event_queue_(input_event_queue),
                                                                         output_event_queue_(output_event_queue) {
    }

    void manipulate(void) {
      manipulator_manager_.manipulate(input_event_queue_.lock(),
                                      output_event_queue_.lock());
    }

    void invalidate_manipulators(void) {
      manipulator_manager_.invalidate_manipulators();
    }

    bool needs_input_event_delay(void) const {
      return manipulator_manager_.needs_input_event_delay();
    }

    bool needs_virtual_hid_pointing(void) const {
      return manipulator_manager_.needs_virtual_hid_pointing();
    }

    void log_events_sizes(void) const {
      if (auto ieq = input_event_queue_.lock()) {
        if (auto oeq = output_event_queue_.lock()) {
          logger::get_logger().info("connection events sizes: {0} -> {1}",
                                    ieq->get_events().size(),
                                    oeq->get_events().size());
        }
      }
    }

  private:
    manipulator_manager& manipulator_manager_;
    std::weak_ptr<event_queue> input_event_queue_;
    std::weak_ptr<event_queue> output_event_queue_;
  };

  manipulator_managers_connector(void) {
  }

  void emplace_back_connection(manipulator_manager& manipulator_manager,
                               const std::shared_ptr<event_queue>& input_event_queue,
                               const std::shared_ptr<event_queue>& output_event_queue) {
    connections_.emplace_back(manipulator_manager,
                              input_event_queue,
                              output_event_queue);
    last_output_event_queue_ = output_event_queue;
  }

  void emplace_back_connection(manipulator_manager& manipulator_manager,
                               const std::shared_ptr<event_queue>& output_event_queue) {
    if (auto ieq = last_output_event_queue_.lock()) {
      connections_.emplace_back(manipulator_manager,
                                ieq,
                                output_event_queue);
      last_output_event_queue_ = output_event_queue;
    } else {
      throw std::runtime_error("last_output_event_queue_ is invalid");
    }
  }

  void manipulate(void) {
    for (auto&& c : connections_) {
      c.manipulate();
    }
  }

  void invalidate_manipulators(void) {
    for (auto&& c : connections_) {
      c.invalidate_manipulators();
    }
  }

  bool needs_input_event_delay(void) const {
    return std::any_of(std::begin(connections_),
                       std::end(connections_),
                       [](auto& c) {
                         return c.needs_input_event_delay();
                       });
  }

  bool needs_virtual_hid_pointing(void) const {
    return std::any_of(std::begin(connections_),
                       std::end(connections_),
                       [](auto& c) {
                         return c.needs_virtual_hid_pointing();
                       });
  }

  void log_events_sizes(void) const {
    for (auto&& c : connections_) {
      c.log_events_sizes();
    }
  }

private:
  std::vector<connection> connections_;
  std::weak_ptr<event_queue> last_output_event_queue_;
};
} // namespace manipulator
} // namespace krbn
