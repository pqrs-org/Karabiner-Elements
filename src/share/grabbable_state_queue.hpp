#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include "types.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include <memory>

namespace krbn {
class grabbable_state_queue final {
public:
  const int max_entries = 32;

  grabbable_state_queue(void) : grabbable_states_(max_entries) {
  }

  boost::optional<uint64_t> get_first_grabbed_event_time_stamp(void) const {
    return first_grabbed_event_time_stamp_;
  }

  std::shared_ptr<grabbable_state> find_current_grabbable_state(void) const {
    if (grabbable_states_.empty()) {
      return nullptr;
    }

    return grabbable_states_.back();
  }

  void clear(void) {
    grabbable_states_.clear();
    first_grabbed_event_time_stamp_ = boost::none;
  }

  void push_back_grabbable_state(std::shared_ptr<grabbable_state> state) {
    // Ignore if the first grabbed event is already arrived.
    if (first_grabbed_event_time_stamp_ &&
        *first_grabbed_event_time_stamp_ <= state->get_time_stamp()) {
      return;
    }

    if (grabbable_states_.full()) {
      grabbable_states_.pop_front();
    }

    grabbable_states_.push_back(state);
  }

  void update_first_grabbed_event_time_stamp(uint64_t time_stamp) {
    if (first_grabbed_event_time_stamp_) {
      return;
    }

    first_grabbed_event_time_stamp_ = time_stamp;

    logger::get_logger().info("first grabbed event: time_stamp:{1}",
                              time_stamp);

    // Erase states after first_grabbed_event_time_stamp_.
    grabbable_states_.erase(std::remove_if(std::begin(grabbable_states_),
                                           std::end(grabbable_states_),
                                           [&](const auto& s) {
                                             return s->get_time_stamp() >= time_stamp;
                                           }),
                            std::end(grabbable_states_));
  }

private:
  // Keep multiple entries for when `push_back_entry` is called multiple times before `set_first_grabbed_event_time_stamp`.
  // (We should remove entries after first_grabbed_event_time_stamp_.)

  boost::circular_buffer<std::shared_ptr<grabbable_state>> grabbable_states_;
  boost::optional<uint64_t> first_grabbed_event_time_stamp_;
};
} // namespace krbn
