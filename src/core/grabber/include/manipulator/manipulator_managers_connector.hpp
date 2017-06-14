#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_manager.hpp"
#include <spdlog/spdlog.h>

namespace krbn {
namespace manipulator {
class manipulator_managers_connector final {
public:
  class connection final {
  public:
    connection(manipulator_manager& manipulator_manager,
               event_queue& input_event_queue,
               event_queue& output_event_queue) : manipulator_manager_(manipulator_manager),
                                                  input_event_queue_(input_event_queue),
                                                  output_event_queue_(output_event_queue) {
    }

    void manipulate(void) {
      manipulator_manager_.manipulate(input_event_queue_,
                                      output_event_queue_);
    }

    void invalidate_manipulators(void) {
      manipulator_manager_.invalidate_manipulators();
    }

    void log_events_sizes(spdlog::logger& logger) const {
      logger.info("connection events sizes: {0} -> {1}",
                  input_event_queue_.get_events().size(),
                  output_event_queue_.get_events().size());
    }

  private:
    manipulator_manager& manipulator_manager_;
    event_queue& input_event_queue_;
    event_queue& output_event_queue_;
  };

  manipulator_managers_connector(void) : last_output_event_queue_(nullptr) {
  }

  void emplace_back_connection(manipulator_manager& manipulator_manager,
                               event_queue& input_event_queue,
                               event_queue& output_event_queue) {
    connections_.emplace_back(manipulator_manager,
                              input_event_queue,
                              output_event_queue);
    last_output_event_queue_ = &output_event_queue;
  }

  void emplace_back_connection(manipulator_manager& manipulator_manager,
                               event_queue& output_event_queue) {
    if (!last_output_event_queue_) {
      throw std::runtime_error("last_output_event_queue_ is nullptr");
    }

    connections_.emplace_back(manipulator_manager,
                              *last_output_event_queue_,
                              output_event_queue);
    last_output_event_queue_ = &output_event_queue;
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

  void log_events_sizes(spdlog::logger& logger) const {
    for (auto&& c : connections_) {
      c.log_events_sizes(logger);
    }
  }

private:
  std::vector<connection> connections_;
  event_queue* last_output_event_queue_;
};
} // namespace manipulator
} // namespace krbn
