#pragma once

#include "manipulator/manipulator_factory.hpp"

namespace krbn {
namespace manipulator {
class manipulator_manager final {
public:
  void push_back_manipulator(const nlohmann::json& json) {
    manipulators_.push_back(manipulator_factory::make_manipulator(json));
  }

  void push_back_manipulator(std::unique_ptr<details::base> ptr) {
    manipulators_.push_back(std::move(ptr));
  }

  void manipulate(event_queue& input_event_queue,
                  event_queue& output_event_queue,
                  uint64_t time_stamp) {
    while (!input_event_queue.empty()) {
      auto& front_input_event = input_event_queue.get_front_event();

      if (front_input_event.get_lazy()) {
        bool lazy_events = true;

        // Find a not lazy event
        for (const auto& e : input_event_queue.get_events()) {
          if (!e.get_lazy()) {
            lazy_events = false;
          }
        }

        if (lazy_events) {
          break;
        }
      }

      for (auto&& m : manipulators_) {
        m->manipulate(front_input_event,
                      input_event_queue,
                      output_event_queue,
                      time_stamp);
      }

      if (input_event_queue.get_front_event().get_valid()) {
        output_event_queue.push_back_event(input_event_queue.get_front_event());
      }

      input_event_queue.erase_front_event();
    }

    remove_invalid_manipulators();
  }

  void run_device_ungrabbed_callback(device_id device_id,
                                     event_queue& output_event_queue,
                                     uint64_t time_stamp) {
    for (auto&& m : manipulators_) {
      m->device_ungrabbed_callback(device_id,
                                   output_event_queue,
                                   time_stamp);
    }

    remove_invalid_manipulators();
  }

  void invalidate_manipulators(void) {
    for (auto&& m : manipulators_) {
      m->set_valid(false);
    }

    remove_invalid_manipulators();
  }

  size_t get_manipulators_size(void) {
    return manipulators_.size();
  }

private:
  void remove_invalid_manipulators(void) {
    manipulators_.erase(std::remove_if(std::begin(manipulators_),
                                       std::end(manipulators_),
                                       [](const auto& it) {
                                         // Keep active manipulators.
                                         return !it->get_valid() && !it->active();
                                       }),
                        std::end(manipulators_));
  }

  std::vector<std::unique_ptr<details::base>> manipulators_;
};
} // namespace manipulator
} // namespace krbn
