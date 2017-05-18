#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_manager.hpp"

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

    void manipulate(uint64_t time_stamp) {
      manipulator_manager_.manipulate(input_event_queue_,
                                      output_event_queue_,
                                      time_stamp);
    }

    void run_device_ungrabbed_callback(device_id device_id,
                                       uint64_t time_stamp) {
      manipulator_manager_.run_device_ungrabbed_callback(device_id,
                                                         output_event_queue_,
                                                         time_stamp);
    }

    void invalidate_manipulators(void) {
      manipulator_manager_.invalidate_manipulators();
    }

  private:
    manipulator_manager& manipulator_manager_;
    event_queue& input_event_queue_;
    event_queue& output_event_queue_;
  };

  void emplace_back_connection(manipulator_manager& manipulator_manager,
                               event_queue& input_event_queue,
                               event_queue& output_event_queue) {
    connections_.emplace_back(manipulator_manager,
                              input_event_queue,
                              output_event_queue);
  }

  void manipulate(uint64_t time_stamp) {
    for (auto&& c : connections_) {
      c.manipulate(time_stamp);
    }
  }

  void run_device_ungrabbed_callback(device_id device_id,
                                     uint64_t time_stamp) {
    for (auto&& c : connections_) {
      // Do `manipulate` for previous connection's `run_device_ungrabbed_callback` result.
      c.manipulate(time_stamp);
      c.run_device_ungrabbed_callback(device_id, time_stamp);
    }
  }

  void invalidate_manipulators(void) {
    for (auto&& c : connections_) {
      c.invalidate_manipulators();
    }
  }

private:
  std::vector<connection> connections_;
};
} // namespace manipulator
} // namespace krbn
