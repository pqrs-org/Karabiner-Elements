#pragma once

// `krbn::grabbable_state_queue` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "event_queue.hpp"
#include "types.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>

namespace krbn {
class grabbable_state_queue final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(boost::optional<grabbable_state>)> grabbable_state_changed;

  // Methods

  const int max_entries = 32;

  grabbable_state_queue(const grabbable_state_queue&) = delete;

  grabbable_state_queue(void) : dispatcher_client(),
                                grabbable_states_(max_entries) {
  }

  virtual ~grabbable_state_queue(void) {
    detach_from_dispatcher([] {
    });
  }

  boost::optional<absolute_time> get_first_grabbed_event_time_stamp(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return first_grabbed_event_time_stamp_;
  }

  boost::optional<grabbable_state> find_current_grabbable_state(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return find_current_grabbable_state_();
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto old_state = find_current_grabbable_state_();

    grabbable_states_.clear();
    first_grabbed_event_time_stamp_ = boost::none;

    auto new_state = find_current_grabbable_state_();

    call_grabbable_state_changed_if_needed(old_state, new_state);
  }

  bool push_back_grabbable_state(const grabbable_state& state) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Ignore if the first grabbed event is already arrived.

    if (first_grabbed_event_time_stamp_ &&
        *first_grabbed_event_time_stamp_ <= state.get_time_stamp()) {
      return false;
    }

    // push_back

    auto old_state = find_current_grabbable_state_();

    grabbable_states_.push_back(state);

    auto new_state = find_current_grabbable_state_();

    call_grabbable_state_changed_if_needed(old_state, new_state);

    return true;
  }

  bool update_first_grabbed_event_time_stamp(absolute_time time_stamp) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (first_grabbed_event_time_stamp_) {
      return false;
    }

    // Pointing device sometimes send event with time_stamp(0) just after the device is connected.
    // We have to ignore the event, so compare the time_stamp with the first queued event.

    if (grabbable_states_.empty()) {
      return false;
    }
    if (grabbable_states_.front().get_time_stamp() > time_stamp) {
      return false;
    }

    // Set first_grabbed_event_time_stamp_

    first_grabbed_event_time_stamp_ = time_stamp;

    auto old_state = find_current_grabbable_state_();

    // Erase states after first_grabbed_event_time_stamp_.
    grabbable_states_.erase(std::remove_if(std::begin(grabbable_states_),
                                           std::end(grabbable_states_),
                                           [&](const auto& s) {
                                             return s.get_time_stamp() >= time_stamp;
                                           }),
                            std::end(grabbable_states_));

    auto new_state = find_current_grabbable_state_();

    call_grabbable_state_changed_if_needed(old_state, new_state);

    return true;
  }

  void unset_first_grabbed_event_time_stamp(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    first_grabbed_event_time_stamp_ = boost::none;
  }

private:
  boost::optional<grabbable_state> find_current_grabbable_state_(void) const {
    if (grabbable_states_.empty()) {
      return boost::none;
    }

    return grabbable_states_.back();
  }

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

    enqueue_to_dispatcher([this, new_state] {
      grabbable_state_changed(new_state);
    });
  }

  // Keep multiple entries for when `push_back_entry` is called multiple times before `set_first_grabbed_event_time_stamp`.
  // (We should remove entries after first_grabbed_event_time_stamp_.)
  boost::circular_buffer<grabbable_state> grabbable_states_;

  boost::optional<absolute_time> first_grabbed_event_time_stamp_;

  mutable std::mutex mutex_;
};
} // namespace krbn
