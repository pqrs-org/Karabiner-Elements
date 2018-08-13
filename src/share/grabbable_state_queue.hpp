#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include "types.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>

namespace krbn {
class grabbable_state_queue final {
public:
  // Signals

  boost::signals2::signal<void(boost::optional<grabbable_state>)>
      grabbable_state_changed;

  // Methods

  const int max_entries = 32;

  grabbable_state_queue(void) : grabbable_states_(max_entries) {
  }

  boost::optional<uint64_t> get_first_grabbed_event_time_stamp(void) const {
    return first_grabbed_event_time_stamp_;
  }

  boost::optional<grabbable_state> find_current_grabbable_state(void) const {
    if (grabbable_states_.empty()) {
      return boost::none;
    }

    return grabbable_states_.back();
  }

  void clear(void) {
    auto old_state = find_current_grabbable_state();

    grabbable_states_.clear();
    first_grabbed_event_time_stamp_ = boost::none;

    auto new_state = find_current_grabbable_state();

    call_grabbable_state_changed_if_needed(old_state, new_state);
  }

  bool push_back_grabbable_state(const grabbable_state& state) {
    // Ignore if the first grabbed event is already arrived.
    if (first_grabbed_event_time_stamp_ &&
        *first_grabbed_event_time_stamp_ <= state.get_time_stamp()) {
      return false;
    }

    auto old_state = find_current_grabbable_state();

    grabbable_states_.push_back(state);

    auto new_state = find_current_grabbable_state();

    call_grabbable_state_changed_if_needed(old_state, new_state);

    return true;
  }

  bool update_first_grabbed_event_time_stamp(uint64_t time_stamp) {
    if (first_grabbed_event_time_stamp_) {
      return false;
    }

    first_grabbed_event_time_stamp_ = time_stamp;

    logger::get_logger().info("first grabbed event: time_stamp:{0}",
                              time_stamp);

    auto old_state = find_current_grabbable_state();

    // Erase states after first_grabbed_event_time_stamp_.
    grabbable_states_.erase(std::remove_if(std::begin(grabbable_states_),
                                           std::end(grabbable_states_),
                                           [&](const auto& s) {
                                             return s.get_time_stamp() >= time_stamp;
                                           }),
                            std::end(grabbable_states_));

    auto new_state = find_current_grabbable_state();

    call_grabbable_state_changed_if_needed(old_state, new_state);

    return true;
  }

  void unset_first_grabbed_event_time_stamp(void) {
    first_grabbed_event_time_stamp_ = boost::none;
  }

private:
  void call_grabbable_state_changed_if_needed(boost::optional<grabbable_state> old_state,
                                              boost::optional<grabbable_state> new_state) {
    if (!old_state &&
        !new_state) {
      return;
    }

    if (old_state &&
        new_state &&
        old_state->equals_except_time_stamp(*new_state)) {
      return;
    }

    grabbable_state_changed(new_state);
  }

  // Keep multiple entries for when `push_back_entry` is called multiple times before `set_first_grabbed_event_time_stamp`.
  // (We should remove entries after first_grabbed_event_time_stamp_.)
  boost::circular_buffer<grabbable_state> grabbable_states_;

  boost::optional<uint64_t> first_grabbed_event_time_stamp_;
};
} // namespace krbn
