#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "grabbable_state_queue.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
auto registry_entry_id1 = krbn::registry_entry_id(1);
} // namespace

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("grabbable_state_queue") {
  {
    krbn::grabbable_state_queue queue;

    boost::optional<krbn::grabbable_state> last_changed_grabbable_state;
    int grabbable_state_changed_count = 0;

    queue.grabbable_state_changed.connect([&](auto&& grabbable_state) {
      last_changed_grabbable_state = grabbable_state;
      ++grabbable_state_changed_count;
    });

    REQUIRE(!queue.find_current_grabbable_state());
    REQUIRE(!queue.get_first_grabbed_event_time_stamp());

    queue.clear();
    REQUIRE(grabbable_state_changed_count == 0);

    for (auto time_stamp = 1000ULL; time_stamp < 10000ULL; time_stamp += 1000ULL) {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  time_stamp);
      REQUIRE(queue.push_back_grabbable_state(state));

      REQUIRE(*(queue.find_current_grabbable_state()) == state);
    }

    // Check `grabbable_state_changed` signal
    {
      krbn::grabbable_state expected(registry_entry_id1,
                                     krbn::grabbable_state::state::grabbable,
                                     krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                     1000ULL);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == expected);
      REQUIRE(grabbable_state_changed_count == 1);
    }

    // Remove states after first_grabbed_event_time_stamp_.
    REQUIRE(queue.update_first_grabbed_event_time_stamp(5000ULL));
    REQUIRE(*(queue.get_first_grabbed_event_time_stamp()) == 5000ULL);
    REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == 4000ULL);

    REQUIRE(!queue.update_first_grabbed_event_time_stamp(3000ULL));
    REQUIRE(*(queue.get_first_grabbed_event_time_stamp()) == 5000ULL);
    REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == 4000ULL);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(grabbable_state_changed_count == 1);

    // Ignore events after first_grabbed_event_time_stamp_.
    {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::grabbable,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                  6000ULL);
      REQUIRE(!queue.push_back_grabbable_state(state));
      REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == 4000ULL);
    }
    {
      krbn::grabbable_state state(registry_entry_id1,
                                  krbn::grabbable_state::state::ungrabbable_temporarily,
                                  krbn::grabbable_state::ungrabbable_temporarily_reason::key_repeating,
                                  4500ULL);
      REQUIRE(queue.push_back_grabbable_state(state));
      REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == 4500ULL);

      // Check `grabbable_state_changed` signal
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == state);
      REQUIRE(grabbable_state_changed_count == 2);
    }

    // Unset first_grabbed_event_time_stamp_.
    queue.unset_first_grabbed_event_time_stamp();
    REQUIRE(!queue.get_first_grabbed_event_time_stamp());

    REQUIRE(queue.update_first_grabbed_event_time_stamp(3000ULL));
    REQUIRE(*(queue.get_first_grabbed_event_time_stamp()) == 3000ULL);
    REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == 2000ULL);

    // Check `grabbable_state_changed` signal
    {
      krbn::grabbable_state expected(registry_entry_id1,
                                     krbn::grabbable_state::state::grabbable,
                                     krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                     2000ULL);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      REQUIRE(last_changed_grabbable_state == expected);
      REQUIRE(grabbable_state_changed_count == 3);
    }

    // Clear
    queue.clear();

    // Check `grabbable_state_changed` signal
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(last_changed_grabbable_state == boost::none);
    REQUIRE(grabbable_state_changed_count == 4);
  }
}

TEST_CASE("grabbable_state_queue.circular_buffer") {
  krbn::grabbable_state_queue queue;

  for (int i = 0; i < 10000; ++i) {
    queue.push_back_grabbable_state(krbn::grabbable_state(registry_entry_id1,
                                                          krbn::grabbable_state::state::grabbable,
                                                          krbn::grabbable_state::ungrabbable_temporarily_reason::none,
                                                          i));

    REQUIRE(queue.find_current_grabbable_state()->get_time_stamp() == i);
  }
}
