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

  void manipulate(event_queue& event_queue, uint64_t time_stamp) {
    for (auto&& m : manipulators_) {
      m->manipulate(event_queue, time_stamp);
    }

    // Remove invalid manipulators
    manipulators_.erase(std::remove_if(std::begin(manipulators_),
                                       std::end(manipulators_),
                                       [](const auto& it) {
                                         // Keep active manipulators.
                                         return !it->get_valid() && !it->active();
                                       }),
                        std::end(manipulators_));
  }

private:
  std::vector<std::unique_ptr<details::base>> manipulators_;
};
} // namespace manipulator
} // namespace krbn
