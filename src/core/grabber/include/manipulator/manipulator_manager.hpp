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

  void manipulate(event_queue& event_queue,
                  uint64_t time_stamp) {
    for (auto&& m : manipulators_) {
      m->manipulate(event_queue,
                    time_stamp);
    }

    remove_invalid_manipulators();
  }

  void inactivate_by_device_id(event_queue& event_queue,
                               device_id device_id,
                               uint64_t time_stamp) {
    for (auto&& m : manipulators_) {
      m->inactivate_by_device_id(event_queue,
                                 device_id,
                                 time_stamp);
    }

    remove_invalid_manipulators();
  }

  void invalidate(void) {
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
